from playwright.sync_api import sync_playwright
import os

def run():
    # Ensure verification directory exists
    os.makedirs("verification", exist_ok=True)

    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True)
        page = browser.new_page()

        # Load the local HTML file
        # Note: Since this is just a static file check, some JS might fail if it depends on backend
        # But we are checking the existence of UI elements.
        cwd = os.getcwd()
        page.goto(f"file://{cwd}/html/index.html")

        # Wait for the navbar to be visible
        page.wait_for_selector("#navbar")

        # Check if the ESP-NOW tab is present
        espnow_tab = page.locator("#li-espnow")
        if not espnow_tab.is_visible():
            print("ESP-NOW tab not visible!")
        else:
            print("ESP-NOW tab found.")

        # Click the tab to make the section visible (simulated)
        # Note: Bootstrap tabs usually require JS. Since we can't easily mock the whole backend/JS env
        # for a static file open without a server, we might just inspect the DOM for the section.

        # Check if the ESP-NOW configuration section exists in DOM
        espnow_section = page.locator("#espnow")
        if not espnow_section.count():
             print("ESP-NOW content section not found in DOM!")
        else:
             print("ESP-NOW content section found in DOM.")

        # Take a screenshot of the whole page, forcing the hidden section to be visible for verification
        # We manually remove the 'hidden' class from the #espnow div to verify its layout
        page.evaluate("document.getElementById('espnow').classList.remove('hidden')")
        page.evaluate("document.getElementById('home').classList.add('hidden')")

        page.screenshot(path="verification/espnow_ui.png")
        print("Screenshot saved to verification/espnow_ui.png")

        browser.close()

if __name__ == "__main__":
    run()
