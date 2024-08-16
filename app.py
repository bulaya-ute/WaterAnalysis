from flask import Flask, render_template, request, jsonify
import random

from utils import water_safety_analysis, rgb_to_hex, darken_color, hex_to_rgb

app = Flask(__name__)

# Global variable to store sensor data
data = {
    'turbidity': 0.0,
    'temperature': 0.0,
    'voltage': 0.0,
    'analysis': {
        "safety_rating": 0.528,
        "description": "Drink only in an emergency",
        "color": "#198754",
        "color_alt": rgb_to_hex(darken_color(hex_to_rgb("#198754")))
    }
}


@app.route('/')
def index():
    return render_template('index.html', data=data)


@app.route('/update', methods=['POST'])
def update_data():
    global data
    json_data = request.get_json()
    if json_data:
        data['turbidity'] = json_data.get('turbidity', data['turbidity'])
        data['temperature'] = json_data.get('temperature', data['temperature'])
        data['voltage'] = json_data.get('voltage', data['voltage'])
        analysis_data = water_safety_analysis(data['temperature'], data['voltage'], safe_color="#198754", unsafe_color="#dc3545")
        data['analysis']["safety_percentage"] = json_data.get(analysis_data["safety_rating"], data['analysis']["safety_percentage"])
        data['analysis']["description"] = json_data.get(analysis_data["description"], data['analysis']["description"])
        data['analysis']["color"] = json_data.get(analysis_data["color"], data['analysis']["description"])
        data['analysis']["color_alt"] = json_data.get(analysis_data["color_alt"], data['analysis']["description"])
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
