from flask import Flask, render_template, request, jsonify
import random

from utils import analyze_data, rgb_to_hex, darken_color, hex_to_rgb

app = Flask(__name__)

# Global variable to store sensor data
# data = {
#     'turbidity': 0.0,
#     'temperature': 0.0,
#     'voltage': 0.0,
#     'analysis': {
#         "safety_rating": 0.528,
#         "description": "Drink only in an emergency",
#         "color": "#198754",
#         "color_alt": rgb_to_hex(darken_color(hex_to_rgb("#198754")))
#     }
# }
safe_color = "#198754"
unsafe_color = "#dc3545"

_dummy_data = {'temperature_sensor': {'rawADC': 3817, 'voltage': 2.71, 'temperature': -273.1499939},
               'turbidity_sensor': {'rawADC': 1049, 'voltage': 1.7, 'turbidity': 0.1}}

data = _dummy_data.copy()
data.update({"analysis": analyze_data(temperature=_dummy_data["temperature_sensor"]["temperature"],
                                      turbidity=_dummy_data["turbidity_sensor"]["turbidity"],
                                      safe_color=safe_color, unsafe_color=unsafe_color)})
# data = {
#     "analysis": {
#         "safety_rating": 0.528,
#         "description": "Drink only in an emergency",
#         "color": "#198754",
#         "color_alt": rgb_to_hex(darken_color(hex_to_rgb("#198754")))
#     }
#
# }
#

@app.route('/')
def index():
    return render_template('index.html', data=data)


@app.route('/update', methods=['POST'])
def update_data():
    global data
    json_data = request.get_json()
    if json_data:
        data.update(json_data)
        # data['turbidity'] = json_data.get('turbidity', data['turbidity'])
        # data['temperature'] = json_data.get('temperature', data['temperature'])
        # data['voltage'] = json_data.get('voltage', data['voltage'])
        analysis_data = analyze_data(data['temperature_sensor']["temperature"],
                                     data['turbidity_sensor']["turbidity"],
                                     safe_color=safe_color, unsafe_color=unsafe_color)
        data["analysis"].update(analysis_data)
        print(f"Data received: {json_data}")
        print(f"Analysis data received: {analysis_data}")
        return jsonify({"status": "success"}), 200
    return jsonify({"status": "error"}), 400


@app.route('/data', methods=['GET'])
def get_data():
    global data
    return jsonify(data)


@app.route('/ping', methods=['GET'])
def ping():
    return jsonify({"status": "alive"}), 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
