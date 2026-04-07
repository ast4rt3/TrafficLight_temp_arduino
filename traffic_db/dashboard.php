<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Traffic Controller Dashboard</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;800&family=Share+Tech+Mono&display=swap" rel="stylesheet">
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }

        body {
            font-family: 'Inter', sans-serif;
            background: #0f172a;
            color: #f8fafc;
            min-height: 100vh;
        }

        .topbar {
            background: linear-gradient(135deg, #1e293b 0%, #0f172a 100%);
            border-bottom: 1px solid #334155;
            padding: 20px 40px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .topbar h1 {
            font-size: 22px;
            font-weight: 800;
            color: #38bdf8;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .topbar h1::before {
            content: '🚦';
            font-size: 28px;
        }

        .status-badge {
            background: #059669;
            color: white;
            padding: 6px 16px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
            letter-spacing: 0.5px;
        }

        .container {
            max-width: 1200px;
            margin: 30px auto;
            padding: 0 30px;
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 25px;
        }

        @media (max-width: 900px) {
            .container { grid-template-columns: 1fr; }
        }

        .card {
            background: #1e293b;
            border: 1px solid #334155;
            border-radius: 12px;
            overflow: hidden;
        }

        .card-header {
            background: #334155;
            padding: 16px 20px;
            font-weight: 700;
            font-size: 14px;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            color: #94a3b8;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }

        .card-body { padding: 20px; }

        .full-width { grid-column: 1 / -1; }

        /* Table */
        table {
            width: 100%;
            border-collapse: collapse;
        }

        th {
            text-align: left;
            padding: 12px 16px;
            background: #0f172a;
            color: #64748b;
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 1px;
            border-bottom: 1px solid #334155;
        }

        td {
            padding: 12px 16px;
            border-bottom: 1px solid #1e293b;
            font-size: 14px;
        }

        tr:hover td { background: rgba(56, 189, 248, 0.05); }

        .cmd-text {
            font-family: 'Share Tech Mono', monospace;
            color: #a3e635;
            background: #0f172a;
            padding: 4px 10px;
            border-radius: 4px;
            font-size: 13px;
        }

        .date-text {
            color: #64748b;
            font-size: 12px;
        }

        /* Buttons */
        .btn {
            border: none;
            padding: 7px 14px;
            border-radius: 6px;
            font-size: 12px;
            font-weight: 600;
            cursor: pointer;
            transition: 0.15s;
        }

        .btn-play { background: #10b981; color: white; }
        .btn-play:hover { background: #059669; }
        .btn-edit { background: #3b82f6; color: white; }
        .btn-edit:hover { background: #2563eb; }
        .btn-del { background: #ef4444; color: white; }
        .btn-del:hover { background: #dc2626; }
        .btn-save { background: #f59e0b; color: #0f172a; }
        .btn-save:hover { background: #d97706; }
        .btn-primary { background: #38bdf8; color: #0f172a; padding: 10px 20px; font-size: 14px; }
        .btn-primary:hover { background: #0ea5e9; }

        .actions { display: flex; gap: 6px; }

        /* Settings */
        .setting-row {
            display: flex;
            align-items: center;
            gap: 12px;
            padding: 12px 0;
            border-bottom: 1px solid #334155;
        }

        .setting-row:last-child { border-bottom: none; }

        .setting-label {
            font-size: 13px;
            color: #94a3b8;
            min-width: 120px;
            font-weight: 600;
        }

        .setting-input {
            flex: 1;
            background: #0f172a;
            border: 1px solid #475569;
            color: #f8fafc;
            padding: 10px 14px;
            border-radius: 6px;
            font-family: 'Share Tech Mono', monospace;
            font-size: 14px;
            outline: none;
        }

        .setting-input:focus { border-color: #38bdf8; }

        /* Add Sequence Form */
        .add-form {
            display: flex;
            gap: 10px;
            padding-top: 15px;
            border-top: 1px solid #334155;
            margin-top: 15px;
        }

        .add-form input {
            flex: 1;
            background: #0f172a;
            border: 1px solid #475569;
            color: #f8fafc;
            padding: 10px;
            border-radius: 6px;
            font-size: 13px;
            outline: none;
        }

        .add-form input:focus { border-color: #38bdf8; }

        /* Toast */
        .toast {
            position: fixed;
            bottom: 30px;
            right: 30px;
            background: #059669;
            color: white;
            padding: 14px 24px;
            border-radius: 8px;
            font-size: 14px;
            font-weight: 600;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            transform: translateY(100px);
            opacity: 0;
            transition: 0.3s;
            z-index: 999;
        }

        .toast.show { transform: translateY(0); opacity: 1; }

        .empty-state {
            text-align: center;
            color: #475569;
            padding: 40px;
            font-style: italic;
        }

        /* Stats */
        .stats {
            display: flex;
            gap: 20px;
        }

        .stat-box {
            flex: 1;
            background: #0f172a;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 20px;
            text-align: center;
        }

        .stat-box .number {
            font-size: 36px;
            font-weight: 800;
            color: #38bdf8;
            font-family: 'Share Tech Mono', monospace;
        }

        .stat-box .label {
            font-size: 11px;
            color: #64748b;
            text-transform: uppercase;
            letter-spacing: 1px;
            margin-top: 5px;
        }
    </style>
</head>
<body>

    <div class="topbar">
        <h1>Traffic Controller Database</h1>
        <span class="status-badge" id="dbStatus">Connecting...</span>
    </div>

    <div class="container">
        <!-- Stats -->
        <div class="card full-width">
            <div class="card-body stats">
                <div class="stat-box">
                    <div class="number" id="seqCount">-</div>
                    <div class="label">Saved Sequences</div>
                </div>
                <div class="stat-box">
                    <div class="number" id="settingsCount">-</div>
                    <div class="label">Settings</div>
                </div>
                <div class="stat-box">
                    <div class="number" id="lastUpdate">-</div>
                    <div class="label">Last Updated</div>
                </div>
            </div>
        </div>

        <!-- Sequences Table -->
        <div class="card full-width">
            <div class="card-header">
                <span>Saved Sequences</span>
                <button class="btn btn-primary" onclick="toggleAddForm()">+ Add New</button>
            </div>
            <div class="card-body">
                <table>
                    <thead>
                        <tr>
                            <th>#</th>
                            <th>Name</th>
                            <th>Command</th>
                            <th>Last Updated</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody id="seqBody">
                        <tr><td colspan="5" class="empty-state">Loading...</td></tr>
                    </tbody>
                </table>

                <div class="add-form" id="addForm" style="display:none;">
                    <input type="text" id="newName" placeholder="Sequence Name">
                    <input type="text" id="newCmd" placeholder="Command (e.g. G10,Y5,R10 L)" style="flex:2;">
                    <button class="btn btn-save" onclick="addSequence()">Save</button>
                </div>
            </div>
        </div>

        <!-- Settings -->
        <div class="card">
            <div class="card-header">
                <span>ESP32 Settings</span>
                <button class="btn btn-save" onclick="saveAllSettings()">Save All</button>
            </div>
            <div class="card-body">
                <div class="setting-row">
                    <span class="setting-label">Default CMD</span>
                    <input type="text" class="setting-input" id="set_defaultCmd" placeholder="e.g. G10,Y5,R10 L">
                </div>
                <div class="setting-row">
                    <span class="setting-label">Wi-Fi SSID</span>
                    <input type="text" class="setting-input" id="set_staSSID" placeholder="Home Wi-Fi Name">
                </div>
                <div class="setting-row">
                    <span class="setting-label">Wi-Fi Pass</span>
                    <input type="password" class="setting-input" id="set_staPass" placeholder="Wi-Fi Password">
                </div>
            </div>
        </div>

        <!-- Quick Reference -->
        <div class="card">
            <div class="card-header"><span>Command Reference</span></div>
            <div class="card-body" style="font-size: 13px; color: #94a3b8; line-height: 2;">
                <code class="cmd-text">R</code> Red &nbsp;
                <code class="cmd-text">Y</code> Yellow &nbsp;
                <code class="cmd-text">G</code> Green<br>
                <code class="cmd-text">G10</code> Green for 10s<br>
                <code class="cmd-text">G10,Y3,R10</code> Sequence<br>
                <code class="cmd-text">L</code> Loop forever<br>
                <code class="cmd-text">B</code> Blink last 2s<br>
                <code class="cmd-text">RAPID</code> Rapid flash<br>
                <code class="cmd-text">D</code> Set as power-on default<br>
                <code class="cmd-text">X</code> Stop / All OFF
            </div>
        </div>
    </div>

    <div class="toast" id="toast"></div>

    <script>
        const API = 'api.php';

        function toast(msg, color = '#059669') {
            const t = document.getElementById('toast');
            t.textContent = msg;
            t.style.background = color;
            t.classList.add('show');
            setTimeout(() => t.classList.remove('show'), 2500);
        }

        function formatDate(d) {
            if (!d) return '-';
            const dt = new Date(d);
            return dt.toLocaleDateString() + ' ' + dt.toLocaleTimeString([], {hour:'2-digit', minute:'2-digit'});
        }

        // ==========================================
        // LOAD DATA
        // ==========================================
        async function loadSequences() {
            try {
                const res = await fetch(`${API}?action=list_seqs`);
                const data = await res.json();
                const seqs = data.sequences || [];

                document.getElementById('seqCount').textContent = seqs.length;
                document.getElementById('dbStatus').textContent = 'Database Online';
                document.getElementById('dbStatus').style.background = '#059669';

                const tbody = document.getElementById('seqBody');
                if (seqs.length === 0) {
                    tbody.innerHTML = '<tr><td colspan="5" class="empty-state">No sequences saved yet. Click "+ Add New" to create one.</td></tr>';
                    return;
                }

                let latestDate = '';
                tbody.innerHTML = seqs.map((s, i) => {
                    if (s.updated_at > latestDate) latestDate = s.updated_at;
                    return `<tr>
                        <td style="color:#475569;">${i + 1}</td>
                        <td style="font-weight:600;">${escHtml(s.name)}</td>
                        <td><span class="cmd-text">${escHtml(s.command)}</span></td>
                        <td class="date-text">${formatDate(s.updated_at)}</td>
                        <td>
                            <div class="actions">
                                <button class="btn btn-edit" onclick="renameSeq('${escAttr(s.name)}')">Rename</button>
                                <button class="btn btn-del" onclick="deleteSeq('${escAttr(s.name)}')">Delete</button>
                            </div>
                        </td>
                    </tr>`;
                }).join('');

                document.getElementById('lastUpdate').textContent = latestDate ? formatDate(latestDate) : '-';

            } catch (e) {
                document.getElementById('dbStatus').textContent = 'Database Offline';
                document.getElementById('dbStatus').style.background = '#ef4444';
                document.getElementById('seqBody').innerHTML = '<tr><td colspan="5" class="empty-state">Cannot connect to database. Is XAMPP running?</td></tr>';
            }
        }

        async function loadSettings() {
            try {
                const res = await fetch(`${API}?action=get_settings`);
                const data = await res.json();
                const s = data.settings || {};

                let count = 0;
                ['defaultCmd', 'staSSID', 'staPass'].forEach(key => {
                    const el = document.getElementById('set_' + key);
                    if (el && s[key]) {
                        el.value = s[key].value || '';
                        if (s[key].value) count++;
                    }
                });
                document.getElementById('settingsCount').textContent = count;
            } catch (e) { /* silent */ }
        }

        // ==========================================
        // ACTIONS
        // ==========================================
        function toggleAddForm() {
            const f = document.getElementById('addForm');
            f.style.display = f.style.display === 'none' ? 'flex' : 'none';
            if (f.style.display === 'flex') document.getElementById('newName').focus();
        }

        async function addSequence() {
            const name = document.getElementById('newName').value.trim();
            const cmd = document.getElementById('newCmd').value.trim();
            if (!name || !cmd) return toast('Enter both name and command!', '#ef4444');

            await fetch(`${API}?action=save_seq&name=${encodeURIComponent(name)}&cmd=${encodeURIComponent(cmd)}`);
            document.getElementById('newName').value = '';
            document.getElementById('newCmd').value = '';
            toast(`Sequence "${name}" saved!`);
            loadSequences();
        }

        async function renameSeq(oldName) {
            const newName = prompt(`Rename "${oldName}" to:`, oldName);
            if (!newName || newName === oldName) return;
            await fetch(`${API}?action=rename_seq&old_name=${encodeURIComponent(oldName)}&new_name=${encodeURIComponent(newName)}`);
            toast(`Renamed to "${newName}"`);
            loadSequences();
        }

        async function deleteSeq(name) {
            if (!confirm(`Delete "${name}"?`)) return;
            await fetch(`${API}?action=delete_seq&name=${encodeURIComponent(name)}`);
            toast(`"${name}" deleted`, '#ef4444');
            loadSequences();
        }

        async function saveAllSettings() {
            const keys = ['defaultCmd', 'staSSID', 'staPass'];
            for (const key of keys) {
                const val = document.getElementById('set_' + key).value;
                await fetch(`${API}?action=save_setting&key=${encodeURIComponent(key)}&value=${encodeURIComponent(val)}`);
            }
            toast('Settings saved!');
            loadSettings();
        }

        function escHtml(s) { return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;'); }
        function escAttr(s) { return s.replace(/'/g, "\\'").replace(/"/g, '\\"'); }

        // Initial load
        loadSequences();
        loadSettings();
    </script>
</body>
</html>
