from flask import Flask, render_template, request, jsonify
import random

app = Flask(__name__)

# Global variable to store sensor data
data = {
    'turbidity': 0.0,
    'temperature': 0.0,
    'voltage': 0.0,
    'analysis': {
        "safety_percentage": 52.8,
        "description": "Drink only in an emergency"
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
        data['analysis']["safety_percentage"] = json_data.get('analysis', data['analysis']["safety_percentage"])
        data['analysis']["description"] = json_data.get('description', data['analysis']["description"])
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
