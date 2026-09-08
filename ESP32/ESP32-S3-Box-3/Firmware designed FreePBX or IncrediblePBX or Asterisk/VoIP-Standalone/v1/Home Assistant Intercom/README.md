# Home Assistant SIP Intercom Card Setup

This guide explains how to install and configure a browser-based SIP intercom card in Home Assistant using **VoIP Stack** and, optionally, **FreePBX or IncrediblePBX**.

You can use this setup in two different ways:

1. **Local room-to-room intercom:** Devices call each other only inside your home or local network.
2. **External calling:** Home Assistant can connect through a SIP server such as FreePBX or IncrediblePBX for calling phones, other SIP users, businesses, friends, or family outside your home.

---

## Requirements

Before you begin, make sure you have the following:

- Home Assistant OS with support for **Apps/Add-ons** and **HACS**
- Access to your Home Assistant configuration and SSL setup
- A microphone on every device that will make or receive calls
- Devices and browsers that support HTTPS
- Optional: a camera, if you want video support
- Optional for external calling: **FreePBX** or **IncrediblePBX**
  - IncrediblePBX may be easier to use for some installations because it includes additional firewall and security tools.

---

# Part 1: Decide Which Setup You Need

## Option A: Local Home Intercom Only

If you only want a room-to-room intercom inside your home, you can skip the following:

- FreePBX/IncrediblePBX
- Nginx Proxy Manager
- Cloudflare Tunnel
- Purchasing a domain

However, you **still need HTTPS**. Microphone access in modern browsers and Home Assistant apps generally requires a secure HTTPS connection.

Make sure every device accessing Home Assistant uses an HTTPS-compatible interface and has a working microphone.

Then continue to **Part 3: Install VoIP Stack**.

---

## Option B: Calling Outside Your Home

If you want to make or receive calls outside your home, such as calling:

- Your cell phone
- Friends or family
- Other SIP users
- Businesses
- Other external phone numbers

You will need additional infrastructure, such as:

- FreePBX or IncrediblePBX
- A domain name
- Cloudflare
- Cloudflare Tunnel
- Nginx Proxy Manager (NPM)

The general setup is:

```text
Internet
   |
   v
Cloudflare
   |
   v
Cloudflare Tunnel
   |
   v
Nginx Proxy Manager
   |
   +-------------------+
   |                   |
   v                   v
Home Assistant      FreePBX/IncrediblePBX
```

---

# Part 2: Set Up HTTPS and External Access

## 1. Purchase a Domain

Purchase a domain from a domain registrar of your choice.

## 2. Connect the Domain to Cloudflare

Add your domain to Cloudflare and update your domain's nameservers according to Cloudflare's instructions.

## 3. Install and Configure Cloudflare Tunnel

Create a Cloudflare Tunnel that securely connects your home network to Cloudflare without directly exposing your public IP address.

You can use the tunnel to route your domain or subdomains to services inside your network.

For example:

```text
ha.example.com      -> Home Assistant
pbx.example.com     -> FreePBX/IncrediblePBX
“Word of caution: If you don’t know what you are doing, have no knowledge, no firewall knowledge—never expose your PBX to the world. In this setup, you do not have to expose the PBX to Cloudflare Tunnels, only Home Assistant. Could cost you lots of money from hackers and 911 fee's.”
```

## 4. Install Nginx Proxy Manager

Install **Nginx Proxy Manager (NPM)** and configure proxy hosts for your internal services.

NPM can help route incoming domain requests to the correct device or service on your network.

## 5. Configure HTTPS

Your Home Assistant installation should be accessible through HTTPS.

Example:

```text
https://ha.example.com
```

Do not rely on plain HTTP for browser microphone access.

---

# Part 3: Install VoIP Stack

1. Open **HACS** in Home Assistant.
2. Search for **VoIP Stack**.
3. Install it.
4. Restart Home Assistant if required.
5. Go to:

   **Settings -> Devices & Services**

6. Click **Add Integration**.
7. Search for **VoIP Stack**.
8. Complete the integration setup.

---

# Part 4: Add Browser Phones

Each browser-based phone should have its own extension.

For this example, we will create two phones:

- Extension `123`
- Extension `321`

## Create the First Phone

1. Go to:

   **Settings -> Devices & Services**

2. Open **VoIP Stack**.
3. Click **Add Phone**.
4. Select **Browser**.
5. Assign extension:

   ```text
   123
   ```

6. Save the phone.

## Create the Second Phone

Repeat the process:

1. Click **Add Phone**.
2. Select **Browser**.
3. Assign extension:

   ```text
   321
   ```

4. Save the phone.

You now have two browser phones that can be used as separate intercom endpoints.

---

# Part 5: Connect VoIP Stack to FreePBX or IncrediblePBX

This section is only necessary if you are using FreePBX or IncrediblePBX.

Before configuring Home Assistant, make sure you have already created or obtained a SIP extension and its credentials.

For example:

```text
Extension: 701
Secret/Password: YOUR_SECRET
FreePBX IP: 192.168.1.100
```

> **Important:** The values above are only examples. Use your own extension number, password, and PBX IP address.

## Configure the Connection

1. Go to:

   **Settings -> Devices & Services**

2. Open **VoIP Stack**.
3. Click the **three-dot menu**.
4. Select **Reconfigure**.

Enter your FreePBX or IncrediblePBX information.

Depending on your configuration, you may see fields similar to:

- Trunk
- Trunk Port
- Trunk User
- Auth User
- Password
- Outbound Proxy

Use the information from your SIP extension.

Example:

| Setting | Example |
|---|---|
| SIP Server / Trunk Address | `192.168.1.100` |
| Port | Your configured SIP port |
| Extension / Username | `701` |
| Auth User | `701` |
| Password | Your extension secret |
| Outbound Proxy | `192.168.1.100` |

In this example, `701` is the SIP extension created in FreePBX or IncrediblePBX.

After entering your information, click **Submit**.

> **Note:** SIP settings can vary depending on your PBX configuration, SIP driver, firewall, network, and version of FreePBX/IncrediblePBX.

---

# Part 6: Create a Dashboard for Each Device

Each device or intercom endpoint should have its own Home Assistant dashboard.

For this example, create one dashboard for extension `123` and another for extension `321`.

## Create Dashboard 123

1. Go to:

   **Settings -> Dashboards**

2. Create a new dashboard.
3. Name it:

   ```text
   123
   ```

4. Open the dashboard and click **Edit Dashboard**.
5. Click **Add Card**.
6. Scroll down and select the **VoIP Stack** card.
7. Set **Card Mode** to:

   ```text
   Home Assistant Softphone
   ```

8. Select phone:

   ```text
   123
   ```

9. Save the card.

## Create Dashboard 321

Repeat the same steps:

1. Create a new dashboard named:

   ```text
   321
   ```

2. Add the **VoIP Stack** card.
3. Set **Card Mode** to:

   ```text
   Home Assistant Softphone
   ```

4. Select phone:

   ```text
   321
   ```

5. Save the card.

---

# Part 7: Install Home Assistant on Your Devices

You can use the Home Assistant app or a compatible browser on devices such as:

- Android phones
- iPhones
- iPads
- Android tablets
- Laptops
- Desktop computers

Every device must have:

- Access to your Home Assistant server
- An HTTPS connection
- A working microphone
- Permission to use the microphone

## Device 1

Open the Home Assistant dashboard for:

```text
123
```

## Device 2

Open the Home Assistant dashboard for:

```text
321
```

---

# Part 8: Test the Intercom

On the device using extension `123`:

1. Open the VoIP Stack card.
2. Select extension `321` as the destination.
3. Press the call button.

On the device using extension `321`:

1. Make sure the VoIP Stack card is open.
2. Allow microphone permissions if prompted.
3. Answer the incoming call.

You should now be able to communicate between the two devices.

To test the opposite direction:

- Call `123` from device `321`.

---

# Troubleshooting

## The Microphone Does Not Work

Check the following:

- Make sure you are accessing Home Assistant using **HTTPS**.
- Make sure the browser or Home Assistant app has microphone permission.
- Verify that the device has a working microphone.
- Try another browser if necessary.
- Reload the page after granting microphone permission.

## The Phone Will Not Register

Check:

- Extension number
- Username
- Authentication username
- Password/secret
- PBX IP address
- SIP port
- Firewall rules
- Outbound proxy settings

## The Devices Cannot Call Each Other

Verify that:

- Each browser phone has a unique extension.
- Each dashboard is connected to the correct phone.
- The destination extension is correct.
- VoIP Stack is running correctly.
- Both devices can access Home Assistant through HTTPS.

---

# Example Configuration

```text
Home Assistant
|
+-- VoIP Stack
    |
    +-- Browser Phone 123
    |     |
    |     +-- Dashboard 123
    |           |
    |           +-- Android Phone / Tablet / Computer
    |
    +-- Browser Phone 321
          |
          +-- Dashboard 321
                |
                +-- Android Phone / Tablet / Computer
```

For external SIP or phone connectivity:

```text
Home Assistant VoIP Stack
          |
          v
FreePBX / IncrediblePBX
          |
          v
SIP Provider / Trunk
          |
          v
Friends / Family / Cell Phones / Other Phone Numbers
```

---

# Final Notes

For a simple home intercom, the easiest setup is:

```text
Home Assistant + HTTPS + VoIP Stack + Two Browser Phones
```

You do **not** need FreePBX, IncrediblePBX, Cloudflare Tunnel, a domain, or Nginx Proxy Manager if your goal is only to create a local room-to-room intercom.

If you want to expand the system to support external SIP calling or connect your Home Assistant intercom to a PBX, you can add FreePBX or IncrediblePBX and configure the SIP connection through VoIP Stack.

Always keep your PBX, Home Assistant, passwords, and firewall configuration secure, especially if you allow access from outside your home.
“Word of caution: If you don’t know what you are doing, have no knowledge, no firewall knowledge—never expose your PBX to the world. In this setup, you do not have to expose the PBX to Cloudflare Tunnels, only Home Assistant. Could cost you lots of money from hackers and 911 fee's.”

Side Note: i am still unsure how to connect an ESP to Voip_Stack directly to intercom like the steps above for browser to browser, so right now my ESP's and its firmware all connect directly to IncrediblePBX since EXT 701 connects back to voip_stack and i can cross connect calls back and forth no issue.

Once i learn how to connect ESP directly to voip_stack ill write a new readme.md for both setups so you can decide how you want and also make new ESP firmware folders on github to give users to choice what route they wanna go. i am still knew at all this and still learning each day, so if you know how to do this already please let me know using issues tabs.
