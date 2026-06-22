import sqlite3
import pandas as pd
from sklearn.ensemble import RandomForestClassifier
import joblib

# LOAD DATABASE
conn = sqlite3.connect("database.db")

df = pd.read_sql_query(
    "SELECT * FROM sensor_data",
    conn
)

conn.close()

# AUTO LABELING
def make_label(row):

    hum = row["humidity"]
    press = row["pressure"]
    light = row["light_lux"]

    if hum >= 85 and press <= 937:
        return "Rain"

    elif hum >= 75:
        return "Potential Rain"

    elif light >= 500 and hum <= 70:
        return "Sunny"

    else:
        return "Cloudy"


df["weather_label"] = df.apply(
    make_label,
    axis=1
)

# FEATURE
X = df[
    [
        "temperature",
        "humidity",
        "pressure",
        "light_lux",
        "temp_trend",
        "humidity_trend",
        "pressure_trend",
        "light_trend"
    ]
]

y = df["weather_label"]

# TRAIN RANDOM FOREST
model = RandomForestClassifier(
    n_estimators=100,
    random_state=42
)

model.fit(X, y)

# SAVE MODEL
joblib.dump(
    model,
    "weather_model.pkl"
)

print("Model berhasil dibuat!")