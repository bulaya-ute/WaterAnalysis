from flask import Flask, render_template
import random

app = Flask(__name__)


@app.route('/')
def index():
    data = {
        'turbidity': round(random.uniform(0.0, 100.0), 2),
        'temperature': round(random.uniform(-20.0, 50.0), 2),
        'voltage': round(random.uniform(0.0, 220.0), 2),
        'analysis': round(random.uniform(0.0, 1000.0), 2)
    }
    return render_template('index.html', data=data)


if __name__ == '__main__':
    app.run(debug=True)
