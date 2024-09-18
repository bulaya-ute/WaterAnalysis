import time

from flask import Flask, render_template, request, jsonify

from utils import analyze_data, get_sensor_status, save_to_csv

app = Flask(__name__)

safe_color = "#198754"  # Color for 100% safe
unsafe_color = "#dc3545"  # Color for 0% safe
sensor_timeout = 5  # time in seconds. If elapsed without getting a request from the sensor, it is assumed offline
sensor_last_online = None
temp_offset = 0  # To calibrate the temperature readings

# Global variable to store sensor data
_dummy_data = {'temperature_sensor': {'rawADC': 3817, 'voltage': 2.71, 'temperature': -273.1499939},
               'turbidity_sensor': {'rawADC': 1049, 'voltage': 1.7, 'turbidity': 0.1}}

data = _dummy_data.copy()
data.update({"analysis": analyze_data(temperature=_dummy_data["temperature_sensor"]["temperature"],
                                      turbidity=_dummy_data["turbidity_sensor"]["turbidity"],
                                      safe_color=safe_color, unsafe_color=unsafe_color),
             "sensor": get_sensor_status(timeout=sensor_timeout, last_online=sensor_last_online)})


@app.route('/')
def index():
    return render_template('index.html', data=data)


@app.route('/update', methods=['POST'])
def update_data():
    global data, sensor_last_online
    json_data = request.get_json()
    if json_data:
        data.update(json_data)
        save_to_csv(json_data)
        sensor_last_online = time.time()
        data["temperature_sensor"]["temperature"] += temp_offset
        analysis_data = analyze_data(data['temperature_sensor']["temperature"],
                                     data['turbidity_sensor']["turbidity"],
                                     safe_color=safe_color, unsafe_color=unsafe_color)
        data["analysis"].update(analysis_data)
        print(f"Data received: {json_data}")
        print(f"Analysis data received: {analysis_data}")
        return jsonify({"status": "success", "safety_rating": analysis_data["safety_rating"]}), 200
    return jsonify({"status": "error"}), 400


@app.route('/data', methods=['GET'])
def get_data():
    global data
    # Update global variable containing data
    data["sensor"].update(get_sensor_status(timeout=sensor_timeout,
                                            last_online=sensor_last_online))

    return jsonify(data)


@app.route('/ping', methods=['GET'])
def ping():
    return jsonify({"status": "alive"}), 200


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)

