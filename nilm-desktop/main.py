import sys
from collections import deque, defaultdict

from PyQt6 import QtCore, QtGui, QtWidgets
import pyqtgraph as pg
import pandas as pd

from mqtt_worker import MqttWorker

# ---- App Config ----
BROKER = "localhost"        # поменяй на IP твоего брокера
PORT   = 1883
TOPIC  = "nilm"
BUFFER = 3000               # ~50 мин при 1 Гц на устройство
REFRESH_MS = 200            # обновление графиков

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("NILM Host — Desktop Dashboard")
        self.setMinimumSize(1100, 700)
        self.setWindowIcon(QtGui.QIcon("icons/power.png"))

        # Хранилище: device -> deque(rows)
        self.ring = defaultdict(lambda: deque(maxlen=BUFFER))
        self.last_ts = {}

        # ---- UI ----
        central = QtWidgets.QWidget()
        self.setCentralWidget(central)
        layout = QtWidgets.QVBoxLayout(central)

        # Top bar
        top = QtWidgets.QHBoxLayout()
        layout.addLayout(top)

        self.brokerLabel = QtWidgets.QLabel(f"Broker: {BROKER}:{PORT}")
        self.topicLabel  = QtWidgets.QLabel(f"Topic: {TOPIC}")
        self.statusLabel = QtWidgets.QLabel("Status: starting…")
        for w in (self.brokerLabel, self.topicLabel, self.statusLabel):
            w.setStyleSheet("color:#475569;")
        top.addWidget(self.brokerLabel)
        top.addWidget(self.topicLabel)
        top.addStretch()
        top.addWidget(self.statusLabel)

        # Device selector + KPIs
        row = QtWidgets.QHBoxLayout()
        layout.addLayout(row)

        self.deviceCombo = QtWidgets.QComboBox()
        self.deviceCombo.addItem("<none>")
        self.deviceCombo.currentIndexChanged.connect(self.update_plots)
        row.addWidget(QtWidgets.QLabel("Device:"))
        row.addWidget(self.deviceCombo)
        row.addStretch()

        self.kpiSamples = QtWidgets.QLabel("Samples: —")
        self.kpiV = QtWidgets.QLabel("V_RMS: —")
        self.kpiI = QtWidgets.QLabel("I_RMS: —")
        self.kpiP = QtWidgets.QLabel("P: —")
        for k in (self.kpiSamples, self.kpiV, self.kpiI, self.kpiP):
            k.setStyleSheet("font-weight:600;")
            row.addWidget(k)

        # Plots
        self.plotVoltage = pg.PlotWidget(title="Voltage (RMS)")
        self.plotCurrent = pg.PlotWidget(title="Current (RMS)")
        self.plotPower   = pg.PlotWidget(title="Active Power (W)")

        for p in (self.plotVoltage, self.plotCurrent, self.plotPower):
            p.showGrid(x=True, y=True, alpha=0.2)
            p.setBackground("w")
            p.getAxis("left").setPen(pg.mkPen("#64748B"))
            p.getAxis("bottom").setPen(pg.mkPen("#64748B"))

        self.curveV = self.plotVoltage.plot(pen=pg.mkPen("#2563EB", width=2))
        self.curveI = self.plotCurrent.plot(pen=pg.mkPen("#10B981", width=2))
        self.curveP = self.plotPower.plot(pen=pg.mkPen("#F59E0B", width=2))

        layout.addWidget(self.plotVoltage, 1)
        layout.addWidget(self.plotCurrent, 1)
        layout.addWidget(self.plotPower,   1)

        # Table
        self.table = QtWidgets.QTableWidget(0, 5)
        self.table.setHorizontalHeaderLabels(["timestamp", "device", "v_rms", "i_rms", "p_active"])
        self.table.horizontalHeader().setStretchLastSection(True)
        self.table.setMaximumHeight(220)
        layout.addWidget(self.table)

        # Toolbar: export
        tb = self.addToolBar("Actions")
        actCsv = QtGui.QAction("Export CSV", self)
        actCsv.triggered.connect(self.export_csv)
        tb.addAction(actCsv)
        actParquet = QtGui.QAction("Export Parquet", self)
        actParquet.triggered.connect(self.export_parquet)
        tb.addAction(actParquet)

        # MQTT worker
        self.worker = MqttWorker(BROKER, PORT, TOPIC)
        self.worker.start()

        # Timer for polling queue and refreshing UI
        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.on_tick)
        self.timer.start(REFRESH_MS)

    # ---- Data ingestion ----
    def on_tick(self):
        updated_devices = set()
        while True:
            item = self.worker.get()
            if item is None:
                break
            typ, payload = item
            if typ == "status":
                self.statusLabel.setText(f"Status: {payload}")
            elif typ == "data":
                j = payload
                try:
                    ts = pd.to_datetime(j.get("timestamp"), utc=True)
                except Exception:
                    ts = pd.Timestamp.utcnow().tz_localize("UTC")
                row = {
                    "ts": ts,
                    "device": j.get("device", "unknown"),
                    "v_rms": float(j.get("v_rms", "nan")),
                    "i_rms": float(j.get("i_rms", "nan")),
                    "p_active": float(j.get("p_active", "nan")),
                }
                dev = row["device"]
                self.ring[dev].append(row)
                self.last_ts[dev] = ts
                updated_devices.add(dev)

        # populate combo once devices appear
        if updated_devices and self.deviceCombo.count() == 1:
            self.deviceCombo.clear()
            self.deviceCombo.addItems(sorted(self.ring.keys()))

        # refresh plots if current device received data
        self.update_plots()

    def current_df(self):
        dev = self.deviceCombo.currentText() if self.deviceCombo.count() else "<none>"
        if dev == "<none>" or dev not in self.ring:
            return pd.DataFrame(columns=["ts","device","v_rms","i_rms","p_active"])
        df = pd.DataFrame(self.ring[dev])
        return df.sort_values("ts")

    def update_plots(self):
        df = self.current_df()
        if df.empty:
            for c in (self.curveV, self.curveI, self.curveP):
                c.setData([], [])
            self.kpiSamples.setText("Samples: —")
            self.kpiV.setText("V_RMS: —")
            self.kpiI.setText("I_RMS: —")
            self.kpiP.setText("P: —")
            self.table.setRowCount(0)
            return

        # KPIs
        self.kpiSamples.setText(f"Samples: {len(df)}")
        self.kpiV.setText(f"V_RMS: {df['v_rms'].iloc[-1]:.1f} V")
        self.kpiI.setText(f"I_RMS: {df['i_rms'].iloc[-1]:.3f} A")
        self.kpiP.setText(f"P: {df['p_active'].iloc[-1]:.1f} W")

        # Plots
        x = pd.to_datetime(df["ts"]).astype("int64") // 10**9
        self.curveV.setData(x, df["v_rms"])
        self.curveI.setData(x, df["i_rms"])
        self.curveP.setData(x, df["p_active"])

        # Table (last 200)
        last = df.tail(200).reset_index(drop=True)
        self.table.setRowCount(len(last))
        for r, row in last.iterrows():
            self.table.setItem(r, 0, QtWidgets.QTableWidgetItem(str(row["ts"])))
            self.table.setItem(r, 1, QtWidgets.QTableWidgetItem(row["device"]))
            self.table.setItem(r, 2, QtWidgets.QTableWidgetItem(f"{row['v_rms']:.2f}"))
            self.table.setItem(r, 3, QtWidgets.QTableWidgetItem(f"{row['i_rms']:.3f}"))
            self.table.setItem(r, 4, QtWidgets.QTableWidgetItem(f"{row['p_active']:.2f}"))

    # ---- Export ----
    def export_csv(self):
        df = self.current_df()
        if df.empty:
            return
        path, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Save CSV", "log.csv", "CSV (*.csv)")
        if path:
            df.to_csv(path, index=False)

    def export_parquet(self):
        try:
            import pyarrow  # noqa
        except Exception:
            QtWidgets.QMessageBox.warning(self, "Parquet", "Install pyarrow to export Parquet.")
            return
        df = self.current_df()
        if df.empty:
            return
        path, _ = QtWidgets.QFileDialog.getSaveFileName(self, "Save Parquet", "log.parquet", "Parquet (*.parquet)")
        if path:
            df.to_parquet(path, index=False)

    def closeEvent(self, e):
        try:
            self.worker.stop()
        except Exception:
            pass
        return super().closeEvent(e)

def main():
    app = QtWidgets.QApplication(sys.argv)
    pg.setConfigOptions(antialias=True)
    w = MainWindow()
    w.show()
    sys.exit(app.exec())

if __name__ == "__main__":
    main()