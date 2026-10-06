let currentConfigFile = '';

function switchTab(tabId, el) {
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.nav li').forEach(l => l.classList.remove('active'));
    document.getElementById(tabId).classList.add('active');
    el.classList.add('active');
    document.querySelector('.topbar h2').innerText = el.innerText.replace(/[^\w\s]/gi, '').trim();
    if(tabId === 'dashboard' || tabId === 'test') fetchStatus();
    if(tabId === 'config' && !currentConfigFile) loadForm('config.json');
    if(tabId === 'logs' && !document.getElementById('log-viewer').value) loadLog('log1.txt');
}

async function fetchStatus() {
    try {
        const res = await fetch('/api/status');
        const data = await res.json();
        
        // Dashboard Updates
        document.getElementById('st-cansell').innerText = data.canSell ? "Operativo" : "Fuera de Serv.";
        document.getElementById('st-tank').innerText = data.sensores.nivel_inferior ? "Con Agua" : "Vacío";
        document.getElementById('st-net').innerText = data.sensores.agua_entrada ? "Conectada" : "Sin Red";
        
        // Hardware Test Sensors Updates
        for(let key in data.sensores) {
            let el = document.getElementById('sens-' + key);
            if(el) {
                let state = data.sensores[key]; // backend ya envía true si hay agua
                el.className = 'sensor ' + (state ? 'active' : 'inactive');
                el.querySelector('span').innerText = state ? "ACTIVO" : "INACTIVO";
            }
        }
        
        // Oxigenation Updates
        if (data.oxigeno) {
            let oState = data.oxigeno.estado;
            let oTimeMs = data.oxigeno.tiempo_restante_ms;
            let oTotalMs = oState ? data.oxigeno.duracion_ms : data.oxigeno.intervalo_ms;
            
            document.getElementById('oxi-status').innerText = oState ? "OXIGENANDO (ON)" : "EN REPOSO (OFF)";
            document.getElementById('oxi-status').style.color = oState ? "#2563EB" : "#4B5563";
            
            let secs = Math.floor(oTimeMs / 1000);
            let mins = Math.floor(secs / 60);
            secs = secs % 60;
            document.getElementById('oxi-time').innerText = `${mins}:${secs < 10 ? '0' : ''}${secs}`;
            
            let percent = 100 - ((oTimeMs / oTotalMs) * 100);
            document.getElementById('oxi-progress').style.width = percent + "%";
            document.getElementById('oxi-progress').style.background = oState ? "#3B82F6" : "#9CA3AF";
        }
    } catch(e) {
        console.error("Error fetching status", e);
    }
}

async function toggleRelay(relay, state) {
    let fd = new URLSearchParams();
    fd.append('relay', relay);
    fd.append('state', state);
    try {
        const res = await fetch('/api/test_relay', { method: 'POST', body: fd });
        if(res.status === 401) window.location.href = "/login";
        else showToast("Comando enviado con éxito", "success");
    } catch(e) {
        showToast("Error al enviar comando", "error");
    }
}

async function loadLog(file) {
    try {
        const res = await fetch('/api/logs?file=' + file);
        if(res.status === 401) window.location.href = "/login";
        if(!res.ok) throw new Error("Log no encontrado");
        const text = await res.text();
        document.getElementById('log-viewer').value = text;
    } catch(e) {
        document.getElementById('log-viewer').value = "Error al cargar " + file;
    }
}

async function toggleTestMode(enable) {
    let fd = new URLSearchParams(); fd.append('enable', enable ? 1 : 0);
    let r = await fetch('/api/test_mode', {method: 'POST', body: fd});
    if(r.ok) {
        document.getElementById('test-mode-lbl').innerText = "Modo Prueba: " + (enable ? "ACTIVO" : "INACTIVO");
        showToast("Modo de prueba " + (enable ? "activado" : "desactivado"));
        if(enable) loadButtonTestUI();
    } else showToast("Error", "error");
}

async function loadButtonTestUI() {
    let r = await fetch('/api/config?file=butons.json');
    if(!r.ok) return;
    let data = await r.json();
    let html = '';
    data.botones.forEach(b => {
        html += `<div class="relay">
            <span>Botón ${b.id} (Pin ${b.pin}) - ${b.tipo}</span>
            <div>
            <button onclick="testBtn(${b.id}, 1)">LED ON</button>
            <button onclick="testBtn(${b.id}, 0)" style="background:#EF4444;">LED OFF</button>
            </div>
        </div>`;
    });
    document.getElementById('btn-test-container').innerHTML = html;
}

async function testBtn(id, state) {
    let fd = new URLSearchParams(); fd.append('id', id); fd.append('state', state);
    await fetch('/api/test_button', {method: 'POST', body: fd});
}

let currentConfigData = {};

async function loadForm(file) {
    currentConfigFile = file;
    document.getElementById('calibration-container').style.display = (file === 'butons.json') ? 'block' : 'none';
    let r = await fetch('/api/config?file=' + file);
    if(r.ok) {
        currentConfigData = await r.json();
        renderForm(file, currentConfigData);
    }
}

function renderForm(file, data) {
    let html = `<form id="dyn-form" onsubmit="event.preventDefault(); saveForm();">`;
    
    if (file === 'config.json') {
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">Web Admin</h4>`;
        html += `<label>Usuario: <input type="text" id="w_user" value="${data.web?.user || ''}" class="form-input"></label><br>`;
        html += `<label>Clave: <input type="password" id="w_pass" value="${data.web?.password || ''}" class="form-input"></label><br><hr>`;
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">WiFi Local (STA)</h4>`;
        html += `<label>SSID: <input type="text" id="s_ssid" value="${data.wifi?.sta?.ssid || ''}" class="form-input"></label><br>`;
        html += `<label>Clave: <input type="password" id="s_pass" value="${data.wifi?.sta?.password || ''}" class="form-input"></label><br><hr>`;
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">WiFi Máquina (AP)</h4>`;
        html += `<label>SSID: <input type="text" id="a_ssid" value="${data.wifi?.ap?.ssid || ''}" class="form-input"></label><br>`;
        html += `<label>Clave: <input type="password" id="a_pass" value="${data.wifi?.ap?.password || ''}" class="form-input"></label>`;
    } 
    else if (file === 'hardware.json') {
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">Configuración Lógica</h4>`;
        html += `<label><input type="checkbox" id="h_red" ${data.config?.requiere_red_agua ? 'checked' : ''}> Requiere Red de Agua (Evita llenado si no hay suministro de calle)</label><br>`;
        html += `<label><input type="checkbox" id="h_inv" ${data.sensores?.sensores_agua_invertidos ? 'checked' : ''}> Sensores de Agua Invertidos (Activar si marca agua cuando no hay)</label><br>`;
        html += `<label><input type="checkbox" id="h_nc" ${data.sensores?.flotador_normalmente_cerrado ? 'checked' : ''}> Flotadores Normalmente Cerrados (NC)</label><br><hr>`;
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">Ciclo de Oxigenación</h4>`;
        html += `<label>Intervalo (Minutos): <input type="number" id="h_int" value="${data.config?.intervalo_oxigenacion_min || 0}" class="form-input"></label><br>`;
        html += `<label>Duración (Minutos): <input type="number" id="h_dur" value="${data.config?.duracion_oxigenacion_min || 0}" class="form-input"></label>`;
    }
    else if (file === 'pago.json') {
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">Medio de Pago</h4>`;
        html += `<label>Tipo de Dispositivo: 
            <select id="p_tipo" class="form-input">
                <option value="transbank_im30" ${data.tipo_dispositivo === 'transbank_im30' ? 'selected' : ''}>Transbank IM30</option>
                <option value="none" ${data.tipo_dispositivo === 'none' ? 'selected' : ''}>Ninguno (Gratis)</option>
            </select></label><br>`;
        html += `<label>MKT Llave: <input type="text" id="p_llave" value="${data.mkt_llave || ''}" class="form-input"></label><br>`;
        html += `<label>Identificador POS: <input type="text" id="p_id" value="${data.identificador || ''}" class="form-input"></label>`;
    }
    else if (file === 'butons.json') {
        html += `<h4 style="margin-bottom:10px; color:#1E3A8A;">Botones Físicos</h4><div style="max-height: 400px; overflow-y:auto;">`;
        data.botones.forEach((b, i) => {
            html += `<div style="background:#fff; padding:10px; border-radius:5px; margin-bottom:10px; border:1px solid #ddd;">
                <strong>Botón ID ${b.id} (Pin ${b.pin})</strong><br>
                <label>Tipo: 
                <select id="b_tipo_${i}" class="form-input">
                    <option value="venta" ${b.tipo === 'venta' ? 'selected' : ''}>Venta (Elegir cantidad)</option>
                    <option value="accion" ${b.tipo === 'accion' ? 'selected' : ''}>Acción (Iniciar/Stop)</option>
                </select></label><br>`;
            html += `<label>Acción/Subtipo: <input type="text" id="b_acc_${i}" value="${b.accion || ''}" class="form-input"></label><br>`;
            html += `<label>Texto en Pantalla: <input type="text" id="b_txt_${i}" value="${b.texto || ''}" class="form-input"></label><br>`;
            html += `<label>Precio ($): <input type="number" id="b_monto_${i}" value="${b.monto || 0}" class="form-input"></label><br>`;
            html += `<label>Tiempo Dispensado (Milisegundos): <input type="number" id="b_time_${i}" value="${b.tiempo_ms || 0}" class="form-input"></label>
            </div>`;
        });
        html += `</div>`;
    }
    
    html += `<button type="submit" style="margin-top:15px; background:#10B981; width:100%;">💾 Guardar Cambios</button></form>`;
    document.getElementById('config-form-container').innerHTML = html;
}

async function saveForm() {
    let file = currentConfigFile;
    let data = currentConfigData; // Mutamos el JSON original
    
    if (file === 'config.json') {
        data.web.user = document.getElementById('w_user').value;
        if(document.getElementById('w_pass').value) data.web.password = document.getElementById('w_pass').value;
        data.wifi.sta.ssid = document.getElementById('s_ssid').value;
        if(document.getElementById('s_pass').value) data.wifi.sta.password = document.getElementById('s_pass').value;
        data.wifi.ap.ssid = document.getElementById('a_ssid').value;
        if(document.getElementById('a_pass').value) data.wifi.ap.password = document.getElementById('a_pass').value;
    } 
    else if (file === 'hardware.json') {
        data.config.requiere_red_agua = document.getElementById('h_red').checked;
        data.sensores.sensores_agua_invertidos = document.getElementById('h_inv').checked;
        data.sensores.flotador_normalmente_cerrado = document.getElementById('h_nc').checked;
        data.config.intervalo_oxigenacion_min = parseInt(document.getElementById('h_int').value);
        data.config.duracion_oxigenacion_min = parseInt(document.getElementById('h_dur').value);
    }
    else if (file === 'pago.json') {
        data.tipo_dispositivo = document.getElementById('p_tipo').value;
        data.mkt_llave = document.getElementById('p_llave').value;
        data.identificador = document.getElementById('p_id').value;
    }
    else if (file === 'butons.json') {
        data.botones.forEach((b, i) => {
            b.tipo = document.getElementById('b_tipo_'+i).value;
            b.accion = document.getElementById('b_acc_'+i).value;
            b.texto = document.getElementById('b_txt_'+i).value;
            b.monto = parseInt(document.getElementById('b_monto_'+i).value);
            b.tiempo_ms = parseInt(document.getElementById('b_time_'+i).value);
        });
    }

    let fd = new URLSearchParams(); 
    fd.append('file', file); 
    fd.append('data', JSON.stringify(data));
    
    let r = await fetch('/api/config', {method: 'POST', body: fd});
    if(r.ok) showToast("Configuración guardada", "success");
    else showToast("Error al guardar", "error");
}

async function testDispense() {
    let t = document.getElementById('calib-time').value;
    let fd = new URLSearchParams(); fd.append('time', t);
    let r = await fetch('/api/test_dispense', {method: 'POST', body: fd});
    if(r.ok) showToast("Dispensando...", "success");
}

function downloadSales() {
    window.location.href = '/api/ventas';
}

async function clearSales() {
    if(confirm("¿Estás seguro de que quieres BORRAR permanentemente todas las ventas actuales?")) {
        let r = await fetch('/api/ventas', {method: 'DELETE'});
        if(r.ok) showToast("Ventas borradas con éxito", "success");
        else showToast("Error al borrar", "error");
    }
}

async function rebootESP() {
    if(confirm("¿Deseas reiniciar la máquina ahora? Las ventas en curso se perderán.")) {
        let r = await fetch('/api/reboot', {method: 'POST'});
        if(r.ok) {
            showToast("Reiniciando sistema... espera 10 segundos.", "success");
            setTimeout(() => window.location.reload(), 10000);
        }
    }
}

// Initial fetch
setInterval(fetchStatus, 2000);
fetchStatus();

function showToast(msg, type="success") {
    let t = document.createElement('div');
    t.className = 'toast ' + type;
    t.innerText = msg;
    document.getElementById('toast-container').appendChild(t);
    setTimeout(() => t.classList.add('show'), 10);
    setTimeout(() => {
        t.classList.remove('show');
        setTimeout(() => t.remove(), 300);
    }, 3000);
}

async function doLogout() {
    await fetch('/api/logout', {method: 'POST'});
    window.location.href = "/login";
}
