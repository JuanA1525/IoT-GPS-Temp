import dash
from dash import dcc, html
import dash_leaflet as dl
from dash.dependencies import Input, Output
from flask import Flask, request
import json
import threading
import datetime

# Inicializar Flask y Dash
server = Flask(__name__)
app = dash.Dash(__name__, server=server)

def decrypt_data(encrypted_hex, key="K3Y$3CR3T"):
    """
    Descifra datos usando XOR con la misma clave que el ESP8266
    """
    try:
        print(f"[DECRYPT] Iniciando descifrado...")
        print(f"[DECRYPT] Datos hex recibidos: {encrypted_hex}")
        print(f"[DECRYPT] Longitud hex: {len(encrypted_hex)} caracteres")
        print(f"[DECRYPT] Clave utilizada: {key}")

        encrypted_bytes = bytes.fromhex(encrypted_hex)
        print(f"[DECRYPT] Bytes cifrados: {encrypted_bytes}")
        print(f"[DECRYPT] Longitud bytes: {len(encrypted_bytes)} bytes")

        decrypted = bytearray()
        key_bytes = key.encode('utf-8')
        key_length = len(key_bytes)
        print(f"[DECRYPT] Clave en bytes: {key_bytes}")
        print(f"[DECRYPT] Longitud clave: {key_length} bytes")

        for i, byte in enumerate(encrypted_bytes):
            key_byte = key_bytes[i % key_length]
            decrypted_byte = byte ^ key_byte
            decrypted.append(decrypted_byte)
            print(f"[DECRYPT] Byte {i}: {byte} XOR {key_byte} = {decrypted_byte} ('{chr(decrypted_byte) if 32 <= decrypted_byte <= 126 else '?'}')")

        decrypted_string = decrypted.decode('utf-8')
        print(f"[DECRYPT] Datos descifrados exitosamente: {decrypted_string}")
        return decrypted_string

    except Exception as e:
        print(f"[DECRYPT] Error al descifrar: {e}")
        print(f"[DECRYPT] Tipo de error: {type(e).__name__}")
        return None

points = {
    'point07': {'lat': 6.242159, 'lon': -75.590510, 'icon': 'punto01.png', 'tooltip': 'PORRAS VELEZ JUAN ANDRES', 'temperatura' : 0, 'humedad': 0},
}
points_lock = threading.Lock()

custom_icon = dict(
    iconUrl='https://static.vecteezy.com/system/resources/previews/026/721/582/original/cute-fat-cat-sticker-design-funny-crazy-cartoon-illustration-png.png',
    shadowUrl='',
    iconSize=[38, 38],
    shadowSize=[50, 64],
    iconAnchor=[22, 94],
    shadowAnchor=[4, 62],
    popupAnchor=[-3, -76]
)

app.layout = html.Div([
    html.H1("EXAMEN 2 IOT: construcción y operación de un End Device"),
    html.P("Envie a la pagina web los datos del sensor mediante un POST para actualizar la posición, temperatura y humedad"),
    dcc.Tabs(id='tabular',value='tabmapa',children=[
        dcc.Tab(label='Mapa',value='tabmapa', children=(
            dl.Map(
        [dl.TileLayer()] + [
            dl.Marker(id=f"marker_{i}",
                      position=(point['lat'], point['lon']),
                      children=dl.Tooltip(point['tooltip']),
                      icon=custom_icon)
            for i, point in points.items()
        ],
        style={'width': '1000px', 'height': '500px'}, center=(6.242159, -75.589913), zoom=16)

        )),
        dcc.Tab(id='Temperatura',label='Temperatura',value='tabtemp', children=(
            dcc.Graph(
            figure={
            'data': [
                {'x': [points['point01']['tooltip'], points['point02']['tooltip'], points['point03']['tooltip'], points['point04']['tooltip'], points['point05']['tooltip'], points['point06']['tooltip'], points['point07']['tooltip'], points['point08']['tooltip'], points['point09']['tooltip'], points['point10']['tooltip'], points['point11']['tooltip'], points['point12']['tooltip'], points['point13']['tooltip'], points['point14']['tooltip'], points['point15']['tooltip'], points['point16']['tooltip'], points['point17']['tooltip'], points['point18']['tooltip'], points['point19']['tooltip'], points['point20']['tooltip'], points['point21']['tooltip']],
                 'y': [points['point01']['temperatura'], points['point02']['temperatura'], points['point03']['temperatura'], points['point04']['temperatura'], points['point05']['temperatura'], points['point06']['temperatura'], points['point07']['temperatura'], points['point08']['temperatura'], points['point09']['temperatura'], points['point10']['temperatura'],points['point11']['temperatura'],points['point12']['temperatura'],points['point13']['temperatura'],points['point14']['temperatura'],points['point15']['temperatura'],points['point16']['temperatura'],points['point17']['temperatura'],points['point18']['temperatura'],points['point19']['temperatura'],points['point20']['temperatura'],points['point21']['temperatura']], 'type': 'bar', 'name': 'Temperatura'},
            ],
            'layout': {
                'title': 'Visualizacion de los datos de temperatura'
                      }
                    }
            )        )),
        dcc.Tab(id='Humedad',label='Humedad',value='tabhum', children=(
            dcc.Graph(
            figure={
            'data': [
                {'x': [points['point01']['tooltip'], points['point02']['tooltip'], points['point03']['tooltip'], points['point04']['tooltip'], points['point05']['tooltip'], points['point06']['tooltip'], points['point07']['tooltip'], points['point08']['tooltip'], points['point09']['tooltip'], points['point10']['tooltip'], points['point11']['tooltip'], points['point12']['tooltip'], points['point13']['tooltip'], points['point14']['tooltip'], points['point15']['tooltip'], points['point16']['tooltip'], points['point17']['tooltip'], points['point18']['tooltip'], points['point19']['tooltip'], points['point20']['tooltip'], points['point21']['tooltip']],
                 'y': [points['point01']['humedad'], points['point02']['humedad'], points['point03']['humedad'], points['point04']['humedad'], points['point05']['humedad'], points['point06']['humedad'], points['point07']['humedad'], points['point08']['humedad'], points['point09']['humedad'], points['point10']['humedad'],points['point11']['humedad'],points['point12']['humedad'],points['point13']['humedad'],points['point14']['humedad'],points['point15']['humedad'],points['point16']['humedad'],points['point17']['humedad'],points['point18']['humedad'],points['point19']['humedad'],points['point20']['humedad'],points['point21']['humedad']], 'type': 'bar', 'name': 'Humedad'},
            ],
            'layout': {
                'title': 'Visualizacion de los datos de humedad'
                      }
                    }
            )        ))

    ]),
    dcc.Interval(id='interval-component', interval=1*1000, n_intervals=0)
])

@server.route('/update_data', methods=['POST'])
def update_data():
    timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print(f"\n{'='*60}")
    print(f"[CONNECTION] Nueva conexión recibida - {timestamp}")
    print(f"[CONNECTION] IP cliente: {request.remote_addr}")
    print(f"[CONNECTION] User-Agent: {request.headers.get('User-Agent', 'No especificado')}")
    print(f"[CONNECTION] Content-Type: {request.headers.get('Content-Type', 'No especificado')}")
    print(f"[CONNECTION] Content-Length: {request.headers.get('Content-Length', 'No especificado')}")

    try:
        raw_data = request.get_data()
        print(f"[DATA] Datos brutos recibidos: {raw_data}")
        print(f"[DATA] Longitud datos brutos: {len(raw_data)} bytes")

        received_data = request.json
        print(f"[DATA] Datos JSON parseados: {received_data}")
        print(f"[DATA] Tipo de datos: {type(received_data)}")

        if received_data:
            print(f"[DATA] Claves en JSON: {list(received_data.keys())}")

        if received_data and 'encrypted_data' in received_data:
            print(f"[ENCRYPTION] Datos cifrados detectados!")
            print(f"[ENCRYPTION] Estructura completa del JSON: {json.dumps(received_data, indent=2)}")

            encrypted_hex = received_data['encrypted_data']
            print(f"[ENCRYPTION] Datos cifrados (hex): {encrypted_hex}")
            print(f"[ENCRYPTION] Iniciando proceso de descifrado...")

            decrypted_json = decrypt_data(encrypted_hex)

            if decrypted_json is None:
                print(f"[ERROR] No se pudieron descifrar los datos")
                return json.dumps({'success': False, 'error': 'Decryption failed'}), 400, {'ContentType': 'application/json'}

            print(f"[ENCRYPTION] Datos descifrados: {decrypted_json}")
            print(f"[ENCRYPTION] Longitud datos descifrados: {len(decrypted_json)} caracteres")

            try:
                data = json.loads(decrypted_json)
                print(f"[ENCRYPTION] JSON descifrado parseado exitosamente: {data}")
                print(f"[ENCRYPTION] Claves en JSON descifrado: {list(data.keys())}")
            except json.JSONDecodeError as e:
                print(f"[ERROR] Error al parsear JSON descifrado: {e}")
                print(f"[ERROR] JSON problemático: '{decrypted_json}'")
                return json.dumps({'success': False, 'error': 'Invalid JSON after decryption'}), 400, {'ContentType': 'application/json'}
        else:
            data = received_data
            print(f"[DATA] Datos no cifrados detectados")
            if data:
                print(f"[DATA] Contenido de datos no cifrados: {data}")

        if data and 'id' in data and 'lat' in data and 'lon' in data:
            print(f"[PROCESSING] Procesando datos para punto {data['id']}")
            print(f"[PROCESSING] Coordenadas: lat={data['lat']}, lon={data['lon']}")
            print(f"[PROCESSING] Temperatura: {data.get('temperatura', 'No especificada')}")
            print(f"[PROCESSING] Humedad: {data.get('humedad', 'No especificada')}")

            with points_lock:
                if data['id'] in points:
                    old_data = points[data['id']].copy()
                    points[data['id']]['lat'] = data['lat']
                    points[data['id']]['lon'] = data['lon']
                    points[data['id']]['temperatura'] = data.get('temperatura', 0)
                    points[data['id']]['humedad'] = data.get('humedad', 0)

                    print(f"[SUCCESS] Datos actualizados para {data['id']}:")
                    print(f"[SUCCESS]   Coordenadas: {old_data['lat']},{old_data['lon']} -> {data['lat']},{data['lon']}")
                    print(f"[SUCCESS]   Temperatura: {old_data['temperatura']} -> {data.get('temperatura', 0)}")
                    print(f"[SUCCESS]   Humedad: {old_data['humedad']} -> {data.get('humedad', 0)}")
                else:
                    print(f"[WARNING] Point ID {data['id']} no encontrado en la lista de puntos")
                    print(f"[WARNING] Puntos disponibles: {list(points.keys())}")
        else:
            print(f"[ERROR] Faltan campos requeridos en los datos")
            if data:
                print(f"[ERROR] Campos recibidos: {list(data.keys())}")
                print(f"[ERROR] Campos requeridos: ['id', 'lat', 'lon']")
            else:
                print(f"[ERROR] No se recibieron datos válidos")
            return json.dumps({'success': False, 'error': 'Missing required fields'}), 400, {'ContentType': 'application/json'}

        print(f"[SUCCESS] Procesamiento completado exitosamente")
        print(f"{'='*60}\n")
        return json.dumps({'success': True}), 200, {'ContentType': 'application/json'}

    except Exception as e:
        print(f"[ERROR] Error general en update_data: {e}")
        print(f"[ERROR] Tipo de error: {type(e).__name__}")
        print(f"[ERROR] Traceback completo:", exc_info=True)
        print(f"{'='*60}\n")
        return json.dumps({'success': False, 'error': str(e)}), 500, {'ContentType': 'application/json'}

@app.callback([Output(f'marker_{i}', 'position') for i in points],
              Input('interval-component', 'n_intervals'))
def update_markers(n):
    with points_lock:
        return [(point['lat'], point['lon']) for point in points.values()]

@app.callback(Output('Temperatura','children'),Input('interval-component', 'n_intervals'))
def update_temperatura(n):
    childrentab=(
            dcc.Graph(
            figure={
            'data': [ {'x':[],'y':[]},
                {'x': [points['point01']['tooltip'], points['point02']['tooltip'], points['point03']['tooltip'], points['point04']['tooltip'], points['point05']['tooltip'], points['point06']['tooltip'], points['point07']['tooltip'], points['point08']['tooltip'], points['point09']['tooltip'], points['point10']['tooltip'], points['point11']['tooltip'], points['point12']['tooltip'], points['point13']['tooltip'], points['point14']['tooltip'], points['point15']['tooltip'], points['point16']['tooltip'], points['point17']['tooltip'], points['point18']['tooltip'], points['point19']['tooltip'], points['point20']['tooltip'], points['point21']['tooltip']],
                 'y': [points['point01']['temperatura'], points['point02']['temperatura'], points['point03']['temperatura'], points['point04']['temperatura'], points['point05']['temperatura'], points['point06']['temperatura'], points['point07']['temperatura'], points['point08']['temperatura'], points['point09']['temperatura'], points['point10']['temperatura'],points['point11']['temperatura'],points['point12']['temperatura'],points['point13']['temperatura'],points['point14']['temperatura'],points['point15']['temperatura'],points['point16']['temperatura'],points['point17']['temperatura'],points['point18']['temperatura'],points['point19']['temperatura'],points['point20']['temperatura'],points['point21']['temperatura']], 'type': 'bar', 'name': 'Temperatura'},
            ],
            'layout': {
                'title': 'Visualizacion de los datos de Temperatura'
                      }
                    }
            )        )
    return childrentab

@app.callback(Output('Humedad','children'),Input('interval-component', 'n_intervals'))
def update_humedad(n):
    childrentab=(
            dcc.Graph(
            figure={
            'data': [
                {'x': [points['point01']['tooltip'], points['point02']['tooltip'], points['point03']['tooltip'], points['point04']['tooltip'], points['point05']['tooltip'], points['point06']['tooltip'], points['point07']['tooltip'], points['point08']['tooltip'], points['point09']['tooltip'], points['point10']['tooltip'], points['point11']['tooltip'], points['point12']['tooltip'], points['point13']['tooltip'], points['point14']['tooltip'], points['point15']['tooltip'], points['point16']['tooltip'], points['point17']['tooltip'], points['point18']['tooltip'], points['point19']['tooltip'], points['point20']['tooltip'], points['point21']['tooltip']],
                 'y': [points['point01']['humedad'], points['point02']['humedad'], points['point03']['humedad'], points['point04']['humedad'], points['point05']['humedad'], points['point06']['humedad'], points['point07']['humedad'], points['point08']['humedad'], points['point09']['humedad'], points['point10']['humedad'],points['point11']['humedad'],points['point12']['humedad'],points['point13']['humedad'],points['point14']['humedad'],points['point15']['humedad'],points['point16']['humedad'],points['point17']['humedad'],points['point18']['humedad'],points['point19']['humedad'],points['point20']['humedad'],points['point21']['humedad']], 'type': 'bar', 'name': 'Humedad'},
            ],
            'layout': {
                'title': 'Visualizacion de los datos de humedad'
                      }
                    }
            )        )
    return childrentab

if __name__ == '__main__':
    print("Iniciando servidor en puerto 1010...")
    print("Dashboard disponible en: http://localhost:1010")
    print("Logs de cifrado activados - Nivel DETALLADO")
    print("="*60)
    app.run(debug=True, port=1010, host='0.0.0.0')
