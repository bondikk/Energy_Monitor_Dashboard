import json
import pandas as pd
from collections import deque
from datetime import datetime

import matplotlib
matplotlib.use("Qt5Agg")   # <— вместо TkAgg
import matplotlib.pyplot as plt

import paho.mqtt.client as mqtt

BROKER = "172.20.10.2"
TOPIC  = "nilm/+/telemetry"

# храним последние N точек, чтобы был «скролл»
WIN = deque(maxlen=600)  # ~10 минут при 1 т/с

def on_connect(c, u, f, rc, properties=None):
    print("[MQTT] Connected:", rc)
    c.subscribe(TOPIC)

def on_message(c, u, m, properties=None):
    j = json.loads(m.payload.decode())
    t = j.get("timestamp") or (datetime.utcnow().isoformat() + "Z")
    WIN.append((t, j["v_rms"], j["i_rms"], j["p_active"]))

def make_df():
    if not WIN:
        return None
    df = pd.DataFrame(WIN, columns=["ts", "v", "i", "p"])
    df["ts"] = pd.to_datetime(df["ts"])
    return df.set_index("ts")

if __name__ == "__main__":
    c = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
    c.on_connect  = on_connect
    c.on_message  = on_message
    c.connect(BROKER, 1883, keepalive=60)
    c.loop_start()

    plt.ion()
    fig, axs = plt.subplots(3, 1, figsize=(9, 6), sharex=True)

    # инициализируем линии один раз
    (lv,) = axs[0].plot([], [], lw=1); axs[0].set_ylabel("V")
    (li,) = axs[1].plot([], [], lw=1); axs[1].set_ylabel("I")
    (lp,) = axs[2].plot([], [], lw=1); axs[2].set_ylabel("P")

    try:
        while True:
            df = make_df()
            if df is not None and len(df) > 1:
                x = df.index.to_pydatetime()
                lv.set_data(x, df["v"].values)
                li.set_data(x, df["i"].values)
                lp.set_data(x, df["p"].values)

                for ax, col in zip(axs, ["v", "i", "p"]):
                    ax.relim();
                    ax.autoscale_view()
                    ax.grid(True, alpha=0.3)

                fig.autofmt_xdate()
                plt.tight_layout()
                fig.canvas.draw()
                fig.canvas.flush_events()

                # 🔽 вот сюда вставь автосохранение:
                if len(WIN) % 60 == 0:  # каждые 500 точек (~8 мин)
                    df.to_csv("log_auto.csv", mode="a", header=False)

            plt.pause(0.2)
    except KeyboardInterrupt:
        out = make_df()
        if out is not None:
            out.to_csv("log.csv")
            print("[LOG] saved -> log.csv")