document.addEventListener('DOMContentLoaded', function() {
    const form = document.getElementById('espnow-form');

    // Load initial settings
    fetch('/api/espnow')
        .then(response => response.json())
        .then(data => {
            form.enabled.checked = data.enabled;
            form.channel.value = data.channel;
            form.mac_address.value = data.mac_address;
            form.priority.value = data.priority;
            form.timeout.value = data.timeout;
        });

    // Save settings
    form.addEventListener('submit', function(event) {
        event.preventDefault();
        const data = {
            enabled: form.enabled.checked,
            channel: parseInt(form.channel.value),
            mac_address: form.mac_address.value,
            priority: form.priority.value,
            timeout: parseInt(form.timeout.value)
        };
        fetch('/api/espnow', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(data)
        });
    });
});
