from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
import sqlite3
from datetime import datetime
import pandas as pd
import joblib

app = Flask(__name__)
CORS(app)

# LOAD AI MODEL
try:
    model = joblib.load("weather_model.pkl")
    print("AI Model Loaded")
except:
    model = None
    print("weather_model.pkl not found")

# CREATE DATABASE
def init_db():

    conn = sqlite3.connect("database.db")
    c = conn.cursor()

    c.execute("""
        CREATE TABLE IF NOT EXISTS sensor_data (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT,
            temperature REAL,
            humidity REAL,
            pressure REAL,
            altitude REAL,
            light_lux REAL,
            temp_trend REAL,
            humidity_trend REAL,
            pressure_trend REAL,
            light_trend REAL,
            device_id TEXT
        )
    """)

    conn.commit()
    conn.close()


init_db()

# HOME
@app.route('/')
def home():
    return "Smart Weather Station API Running"

# DASHBOARD
@app.route('/dashboard')
def dashboard():
    return render_template("dashboard.html")

# RECEIVE DATA FROM ESP32
@app.route('/api/data', methods=['POST'])
def receive_data():

    data = request.get_json()

    print("\n===== DATA RECEIVED =====")
    print(data)
    print("=========================\n")

    conn = sqlite3.connect("database.db")
    c = conn.cursor()

    c.execute("""
        INSERT INTO sensor_data (
            timestamp,
            temperature,
            humidity,
            pressure,
            altitude,
            light_lux,
            temp_trend,
            humidity_trend,
            pressure_trend,
            light_trend,
            device_id
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    """,
    (
        datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        data.get("temperature"),
        data.get("humidity"),
        data.get("pressure"),
        data.get("altitude"),
        data.get("light_lux"),
        data.get("temp_trend"),
        data.get("humidity_trend"),
        data.get("pressure_trend"),
        data.get("light_trend"),
        data.get("device_id")
    ))

    conn.commit()
    conn.close()

    return jsonify({
        "status": "success",
        "message": "Data saved to database"
    }), 200


# HISTORY
@app.route('/api/history')
def history():

    conn = sqlite3.connect("database.db")
    conn.row_factory = sqlite3.Row

    c = conn.cursor()

    c.execute("""
        SELECT *
        FROM sensor_data
        ORDER BY id DESC
    """)

    rows = c.fetchall()
    conn.close()

    result = []

    for row in rows:
        result.append(dict(row))

    return jsonify(result)


# LATEST DATA + AI PREDICTION
@app.route('/api/predict')
def predict():

    conn = sqlite3.connect("database.db")
    conn.row_factory = sqlite3.Row
    c = conn.cursor()

    c.execute("""
        SELECT *
        FROM sensor_data
        ORDER BY id DESC
        LIMIT 1
    """)

    row = c.fetchone()
    conn.close()

    if row is None:
        return jsonify({})

    data = dict(row)

    prediction = "Unknown"

    if model is not None:

        X = pd.DataFrame([{
            "temperature": data["temperature"],
            "humidity": data["humidity"],
            "pressure": data["pressure"],
            "light_lux": data["light_lux"],
            "temp_trend": data["temp_trend"],
            "humidity_trend": data["humidity_trend"],
            "pressure_trend": data["pressure_trend"],
            "light_trend": data["light_trend"]
        }])

        prediction = model.predict(X)[0]

    data["prediction"] = prediction

    return jsonify(data)


# RUN
if __name__ == '__main__':
    app.run(
        host='0.0.0.0',
        port=5000,
        debug=True
    )