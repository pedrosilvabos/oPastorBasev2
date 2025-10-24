## 🐮 oPastor Base Station – Feature Checklist
**Priority:** control, low-power sync, and cloud-driven coordination.

---

### 🛰️ Command & Sync Logic
- [ ] **Send commands from phone → base → specific cow**  
  - Format: `TO:cow_id|CMD:<command>|ARGS:<optional>|ID:<cmdId>`  
  - Commands are delivered during next SYNC window  
  - ACKs handled via `ACK:<cmdId>`  

- [ ] **Optional retry for failed commands**  
  - Retries limited to N cycles  
  - Skipped if cow does not wake or fails to ACK  

---

### 🧠 Pairing & Identity Management
- [ ] **Activate pairing mode** (manually or via timed loop)  
  - Controlled via serial, command, or UI toggle  
  - Auto-disables after timeout  

- [ ] **Allow disabling pairing after boot**  
  - `pairing_enabled = false` as startup flag or config  

- [ ] **Clone existing cow config for test or simulation**  
  - MAC spoofing supported  
  - Flags virtual vs physical  

---

### 📡 Base Autonomy & API
- [ ] **Start Wi-Fi AP on demand**  
  - Triggered via button, command, or fallback mode  
  - Serves web interface for diagnostics and control  

- [ ] **Change polling interval (default: 60s)**  
  - Configurable via web or command  
  - Window scaling logic updates automatically  

- [ ] **Persist cow registry in SPIFFS**  
  - Includes `cow_id`, `mac`, `last_seen`, `flags`  

- [ ] **Track last-seen + battery per cow**  
  - Cached in memory, optionally flushed to SPIFFS  

---

### 💻 User Interface
- [ ] **Web UI in AP mode**  
  - View paired cows, telemetry, and statuses  
  - Trigger commands and pairing  
  - Adjust sync/poll settings  

---

### 🧪 Debug & Resilience
- [ ] **Support debug/test commands from serial**  
  - Example: `SEND TO:cow_3|CMD:BEEP`  

- [ ] **Graceful fallback if cow misses SYNC slot**  
  - Retry once in next rotation  
  - Log or flag if repeated failure  

- [x+] **Remote CLI reset command**  
  - Implemented via PlatformIO custom target (`reset.py`)  
  - Allows `pio run -t reset_bootloader` to trigger board reset without physical access  
  - Useful for remote diagnostics and recovery  

---

### ✅ Confirmations
- [ ] **ACK support for command confirmation**  
  - Required to mark command as completed  
  - `ACK:<cmdId>` via LoRa  
