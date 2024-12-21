import numpy as np
import pickle
import tensorflow as tf
import os
import joblib

# Deaktiviere GPU-Warnungen
os.environ['CUDA_VISIBLE_DEVICES'] = '-1'

def read_system_data_from_proc():
    system_metrics = {}
    try:
        with open('/proc/read_system_data', 'r') as f:
            for line in f.readlines():
                # Extrahiere die Werte aus den Zeilen im Format "Schlüssel: Wert"
                parts = line.strip().split(": ")
                if len(parts) == 2:
                    key, value = parts
                    # Entferne Einheiten und versuche, den Wert in einen numerischen Wert umzuwandeln
                    value = value.replace("kB", "").replace("MHz", "").replace("°C", "").strip()
                    # Speziell für 'CPU Frequency' - entfernen von 'kHz' und Umwandlung in float
                    if 'CPU Frequency' in key:
                        value = value.replace(" kHz", "")  # Entfernt das 'kHz'
                    try:
                        value = float(value)
                    except ValueError:
                        pass
                    system_metrics[key] = value
    except FileNotFoundError:
        print("⚠️ Datei '/proc/read_system_data' nicht gefunden. Stelle sicher, dass das Kernel-Modul geladen ist.")
    return system_metrics

def predict_cpu_frequency():
    print("🚀 Starte Vorhersage-Skript...")

    # Modell laden
    print("📦 Lade Modell...")
    model = tf.keras.models.load_model('cpu_freq_predictor.keras')
    print("✅ Modell erfolgreich geladen.")

    # Scaler laden
    print("📦 Lade Scaler (X und y)...")
    scaler_X = joblib.load("scaler_X.pkl")
    scaler_y = joblib.load("scaler_y.pkl")
    print("✅ Scaler erfolgreich geladen.")

    # Systemdaten aus dem /proc-Dateisystem lesen
    print("📊 Lese Systemdaten...")
    system_metrics = read_system_data_from_proc()

    if not system_metrics:
        print("⚠️ Es konnten keine Systemdaten gelesen werden. Beende Skript.")
        return

    print("📋 Systemmetriken:", system_metrics)

    # Eingabedaten formatieren (Systemmetriken)
    # Verwende nur die 15 richtigen Merkmale
    input_data = np.array([system_metrics[key] for key in [
        'CPU User', 'CPU System', 'CPU Idle', 'Memory Total', 'Memory Free',
        'Swap Total', 'Swap Free', 'Disk Read', 'Disk Write', 'Network RX',
        'Network TX', 'Load 1min', 'Load 5min', 'Load 15min', 'CPU Temperature'
    ]]).reshape(1, -1)

    print(f"🧮 Eingabedaten vor Skalierung: {input_data}")

    # Eingabedaten skalieren
    scaled_input_data = scaler_X.transform(input_data)
    print(f"📏 Eingabedaten nach Skalierung: {scaled_input_data}")

    # Vorhersage auf skalierten Eingabedaten durchführen
    print("🤖 Vorhersage mit dem Modell läuft...")
    scaled_prediction = model.predict(scaled_input_data)

    # Debug-Ausgabe: Vorhersage vor Rückskalierung
    print(f"📏 Vorhersage vor Rückskalierung (skaliert): {scaled_prediction}")

    # Rückskalierung der Vorhersage
    predicted_cpu_frequency = scaler_y.inverse_transform(scaled_prediction.reshape(-1, 1))

    # Setze negative Frequenzen auf 0 (physikalisch nicht möglich)
    predicted_cpu_frequency = np.maximum(predicted_cpu_frequency, 0)

    # Rückskalierte Vorhersage ausgeben
    print(f"🎉 Rückskalierte Vorhersage: {predicted_cpu_frequency[0][0]} MHz")

    actual_cpu_frequency = system_metrics.get('CPU Frequency', 'Nicht verfügbar')
    print(f"🔍 Tatsächliche CPU Frequenz in Echtzeit: {actual_cpu_frequency} MHz")


# Skript starten
if __name__ == "__main__":
    predict_cpu_frequency()

