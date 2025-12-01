import os
import gzip
import re
import json
import glob

# Configuration
INPUT_DIR = 'html'
OUTPUT_FILE = os.path.join('src', 'ESPSWebUI.h')

# File types to process and their MIME types
MIME_TYPES = {
    '.html': 'text/html',
    '.css': 'text/css',
    '.js': 'application/javascript',
    '.json': 'application/json',
    '.png': 'image/png',
    '.ico': 'image/x-icon',
}

# Files to ignore
IGNORE_FILES = [
    'README.md',
    'server.bat',
    'NodeJsServer.cjs',
    'package.json',
    '.DS_Store',
    'thumbs.db'
]

def minify_html(content):
    # Remove comments
    content = re.sub(r'<!--(.*?)-->', '', content, flags=re.DOTALL)
    # Collapse whitespace
    content = re.sub(r'\s+', ' ', content)
    return content

def minify_css(content):
    # Remove comments
    content = re.sub(r'/\*(.*?)\*/', '', content, flags=re.DOTALL)
    # Collapse whitespace
    content = re.sub(r'\s+', ' ', content)
    return content

def minify_json(content):
    try:
        obj = json.loads(content)
        return json.dumps(obj, separators=(',', ':'))
    except:
        return content

def get_c_variable_name(filename):
    # Sanitize filename to be a valid C variable name
    clean = re.sub(r'[^a-zA-Z0-9]', '_', filename)
    return f"WEB_{clean}"

def process_files():
    files_data = []

    for root, dirs, files in os.walk(INPUT_DIR):
        for filename in files:
            if filename in IGNORE_FILES:
                continue

            ext = os.path.splitext(filename)[1].lower()
            if ext not in MIME_TYPES:
                continue

            file_path = os.path.join(root, filename)
            rel_path = os.path.relpath(file_path, INPUT_DIR)
            # Normalize path separators to forward slash for URLs
            url_path = '/' + rel_path.replace(os.sep, '/')

            # Read file content
            try:
                with open(file_path, 'rb') as f:
                    content_bytes = f.read()
            except Exception as e:
                print(f"Error reading {file_path}: {e}")
                continue

            # Minify text-based files
            original_size = len(content_bytes)
            if ext in ['.html', '.css', '.json']: # Skip JS minification for safety
                try:
                    content_str = content_bytes.decode('utf-8')
                    if ext == '.html':
                        content_str = minify_html(content_str)
                    elif ext == '.css':
                        content_str = minify_css(content_str)
                    elif ext == '.json':
                        content_str = minify_json(content_str)
                    content_bytes = content_str.encode('utf-8')
                except Exception as e:
                    print(f"Error minifying {file_path}: {e}")

            # Gzip compress
            compressed_bytes = gzip.compress(content_bytes)

            c_var_name = get_c_variable_name(rel_path)

            files_data.append({
                'url': url_path,
                'mime': MIME_TYPES[ext],
                'var': c_var_name,
                'data': compressed_bytes,
                'orig_size': original_size,
                'comp_size': len(compressed_bytes)
            })

            print(f"Processed {rel_path}: {original_size} -> {len(compressed_bytes)} bytes")

    return files_data

def generate_header(files_data):
    with open(OUTPUT_FILE, 'w') as f:
        f.write('#pragma once\n\n')
        f.write('#include <Arduino.h>\n')
        f.write('#include <ESPAsyncWebServer.h>\n\n')

        # Write byte arrays
        for file in files_data:
            f.write(f"// {file['url']} ({file['orig_size']} bytes -> {file['comp_size']} bytes gzipped)\n")
            f.write(f"const uint8_t {file['var']}[] PROGMEM = {{\n")

            # Write hex data in lines of 16 bytes
            data = file['data']
            for i in range(0, len(data), 16):
                chunk = data[i:i+16]
                hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
                if i + 16 < len(data):
                    hex_str += ','
                f.write(f"    {hex_str}\n")

            f.write("};\n")
            f.write(f"const size_t {file['var']}_len = {len(data)};\n\n")

        # Write init function
        f.write("void InitWebUI(AsyncWebServer& server) {\n")

        for file in files_data:
            url = file['url']
            var_name = file['var']
            var_len = f"{var_name}_len"
            mime = file['mime']

            f.write(f'    server.on("{url}", HTTP_GET, [](AsyncWebServerRequest *request) {{\n')
            f.write(f'        AsyncWebServerResponse *response = request->beginResponse_P(200, "{mime}", {var_name}, {var_len});\n')
            f.write('        response->addHeader("Content-Encoding", "gzip");\n')
            f.write('        request->send(response);\n')
            f.write('    });\n')

            # Special handling for index.html at root
            if url == '/index.html':
                f.write(f'    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {{\n')
                f.write(f'        AsyncWebServerResponse *response = request->beginResponse_P(200, "{mime}", {var_name}, {var_len});\n')
                f.write('        response->addHeader("Content-Encoding", "gzip");\n')
                f.write('        request->send(response);\n')
                f.write('    });\n')

            # Special handling for UpdRecipe.json serving at /UpdRecipe/
            if url == '/UpdRecipe.json':
                 f.write(f'    server.on("/UpdRecipe/", HTTP_GET, [](AsyncWebServerRequest *request) {{\n')
                 f.write(f'        AsyncWebServerResponse *response = request->beginResponse_P(200, "{mime}", {var_name}, {var_len});\n')
                 f.write('        response->addHeader("Content-Encoding", "gzip");\n')
                 f.write('        request->send(response);\n')
                 f.write('    });\n')

        f.write("}\n")

if __name__ == '__main__':
    print("Building Web UI Headers...")
    files_data = process_files()
    generate_header(files_data)
    print(f"Generated {OUTPUT_FILE}")
