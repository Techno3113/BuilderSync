# BuilderSync

**Real-time multiplayer level editing for Geometry Dash — free and open source.**

Build levels together with friends, live. No accounts. No paywalls. Just a room code.

---

## How it works

BuilderSync adds a **Sync** button to the editor toolbar. From there you can:

- **Host** a session — your current level is shared and a 6-letter room code is generated
- **Join** a session — paste a friend's code and drop straight into their level
- **See ghost cursors** — colored crosshairs show where each collaborator is working in real time
- **Leave** at any time without disrupting others

Every object placement, deletion, move, and rotation is broadcast to all connected players instantly over WebSockets.

---

## Setup

### 1. Run a relay server

BuilderSync needs a small relay server to forward messages between players. You can run one yourself for free:

```
cd server/
npm install
node server.js
```

This starts the server on port `2999`. For remote play, open that port on your router or use a tunnel like [Cloudflare Tunnel](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/) (free).

### 2. Configure the mod

In the Geode mod settings for BuilderSync, set:

- **Relay Server URL** — `ws://your-ip:2999` (or `ws://localhost:2999` for local testing)
- **Display Name** — what your collaborators will see

### 3. Start editing

1. Open any level in the GD editor
2. Press **Sync** in the toolbar
3. Press **Connect**, then **Host**
4. Share the room code with your friends
5. Friends press **Sync → Connect → Join** and enter your code

---

## Features

| Feature | Status |
|---|---|
| Object placement sync | ✅ |
| Object deletion sync | ✅ |
| Move sync | ✅ |
| Rotation sync | ✅ |
| Ghost cursors | ✅ |
| Join/leave notifications | ✅ |
| User list | ✅ |
| Late-join level sync | 🚧 In progress |
| Trigger/property sync | 🔜 Planned |
| Undo stack isolation | 🔜 Planned |
| WSS (encrypted) support | 🔜 Planned |

---

## Self-hosting

The relay server is a single Node.js file with one dependency (`ws`). It runs on any machine that can run Node 18+, including free-tier cloud VMs (Railway, Render, Fly.io).

Source: [github.com/Techno3113/BuilderSync](https://github.com/Techno3113/BuilderSync)

---

## FAQ

**Do both players need the same level?**
The host's level is automatically sent to anyone who joins mid-session (full sync is still being improved — for best results, start a session before placing any objects).

**Does this work with Android?**
The mod targets macOS and Windows. Android support is planned but untested.

**Is there a public server?**
Not yet. Run your own — it takes about 30 seconds.

**Will this get me banned?**
BuilderSync only modifies the editor, not gameplay. It does not interact with Robtop's servers in any way.
