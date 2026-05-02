/**
 * BuilderSync Relay Server
 * Run:  node server.js
 * Deps: npm install ws
 */

const { WebSocketServer, WebSocket } = require("ws");
const crypto = require("crypto");

const PORT = process.env.PORT || 2999;
const wss  = new WebSocketServer({ port: PORT });

// rooms: Map<code, Room>
const rooms = new Map();

function makeCode() {
    const chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    return Array.from(crypto.randomBytes(6))
        .map(b => chars[b % chars.length]).join("");
}

function makeUserId() {
    return crypto.randomUUID().split("-")[0];
}

function send(ws, obj) {
    if (ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj));
}

function broadcast(room, obj, exceptId = null) {
    for (const [uid, c] of room.clients) {
        if (uid !== exceptId) send(c.ws, obj);
    }
}

function userList(room) {
    return {
        op: "user_list",
        users: [...room.clients.values()].map(c => ({
            user_id: c.userId,
            name:    c.name,
        })),
    };
}

wss.on("connection", (ws) => {
    const userId = makeUserId();
    let room = null;

    function leave() {
        if (!room) return;
        room.clients.delete(userId);
        console.log(`[${room.code}] ${userId} left — ${room.clients.size} remaining`);

        if (room.clients.size === 0) {
            rooms.delete(room.code);
            console.log(`[${room.code}] dissolved`);
        } else {
            broadcast(room, { op: "left", user_id: userId });
            broadcast(room, userList(room));
            // Promote new host if needed
            if (room.hostId === userId) {
                const next = room.clients.values().next().value;
                room.hostId = next.userId;
                send(next.ws, { op: "promoted_to_host" });
                console.log(`[${room.code}] ${next.name} promoted to host`);
            }
        }
        room = null;
    }

    ws.on("message", (raw) => {
        let pkt;
        try { pkt = JSON.parse(raw); } catch { return; }

        const op = pkt.op;

        // ── host ──────────────────────────────────────────────────────────
        if (op === "host") {
            let code;
            do { code = makeCode(); } while (rooms.has(code));

            room = {
                code,
                hostId:    userId,
                levelData: pkt.level_data || "",
                clients:   new Map([[userId, { ws, userId, name: pkt.name || "Host" }]]),
            };
            rooms.set(code, room);

            send(ws, { op: "hosted", code, user_id: userId });
            console.log(`[${code}] created by ${pkt.name || userId}`);
            return;
        }

        // ── join ──────────────────────────────────────────────────────────
        if (op === "join") {
            const code  = (pkt.code || "").toUpperCase();
            const found = rooms.get(code);
            if (!found) { send(ws, { op: "error", msg: "Room not found." }); return; }

            leave(); // leave any existing room first
            room = found;
            room.clients.set(userId, { ws, userId, name: pkt.name || "Guest" });

            // Confirm to joiner
            send(ws, { op: "joined", code, user_id: userId, self: true });

            // Tell everyone else
            broadcast(room, { op: "joined", user_id: userId, name: pkt.name || "Guest" }, userId);

            // Sync level data to late joiner
            if (room.levelData) {
                send(ws, { op: "level_sync", level_data: room.levelData });
            }

            // Refresh user list for everyone
            const list = userList(room);
            broadcast(room, list);
            send(ws, list);

            console.log(`[${code}] ${pkt.name || userId} joined — ${room.clients.size} users`);
            return;
        }

        // ── leave ─────────────────────────────────────────────────────────
        if (op === "leave") { leave(); return; }

        // ── forward everything else to room peers ─────────────────────────
        if (room) {
            pkt.user_id = userId;
            broadcast(room, pkt, userId);
        }
    });

    ws.on("close", leave);
});

console.log(`BuilderSync relay running on ws://localhost:${PORT}`);