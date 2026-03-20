import { useEffect, useState } from "react";
import { getLatestMeasurement } from "../api.js";

function MetricCard({ title, value, unit }) {
  return (
    <div
      style={{
        border: "1px solid #e5e7eb",
        borderRadius: 16,
        padding: 20,
        minWidth: 180,
        background: "#ffffff",
        boxShadow: "0 4px 16px rgba(0,0,0,0.05)",
      }}
    >
      <div style={{ fontSize: 14, color: "#6b7280", marginBottom: 8 }}>{title}</div>
      <div style={{ fontSize: 32, fontWeight: 700, color: "#111827" }}>
        {value ?? "-"} <span style={{ fontSize: 18, fontWeight: 500 }}>{unit}</span>
      </div>
    </div>
  );
}

export default function Dashboard() {
  const [data, setData] = useState(null);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);

  async function load() {
    try {
      const result = await getLatestMeasurement();
      setData(result);
      setError("");
    } catch (e) {
      setError("Не удалось получить данные с backend");
    } finally {
      setLoading(false);
    }
  }

  useEffect(() => {
    load();
    const id = setInterval(load, 2000);
    return () => clearInterval(id);
  }, []);

  return (
    <div
      style={{
        minHeight: "100vh",
        background: "#f3f4f6",
        padding: 32,
        fontFamily: "Inter, Arial, sans-serif",
      }}
    >
      <div style={{ maxWidth: 1100, margin: "0 auto" }}>
        <h1 style={{ margin: 0, fontSize: 36, color: "#111827" }}>NILM Dashboard</h1>
        <p style={{ color: "#6b7280", marginTop: 8 }}>
          Live telemetry from ESP32 + ADS1256
        </p>

        {loading && <p>Загрузка...</p>}
        {error && (
          <div
            style={{
              marginTop: 16,
              padding: 14,
              borderRadius: 12,
              background: "#fee2e2",
              color: "#991b1b",
            }}
          >
            {error}
          </div>
        )}

        {data && (
          <>
            <div
              style={{
                display: "flex",
                gap: 20,
                flexWrap: "wrap",
                marginTop: 28,
              }}
            >
              <MetricCard title="Current" value={data.i_rms} unit="A" />
              <MetricCard title="Voltage" value={data.voltage_rms} unit="V" />
              <MetricCard title="Apparent Power" value={data.s_est_va} unit="VA" />
            </div>

            <div
              style={{
                marginTop: 28,
                background: "#ffffff",
                border: "1px solid #e5e7eb",
                borderRadius: 16,
                padding: 20,
                boxShadow: "0 4px 16px rgba(0,0,0,0.05)",
              }}
            >
              <h2 style={{ marginTop: 0, color: "#111827" }}>Telemetry Info</h2>
              <p><strong>Device:</strong> {data.device ?? "-"}</p>
              <p><strong>Timestamp:</strong> {data.timestamp ?? "-"}</p>
              <p><strong>ADC SPS:</strong> {data.adc_sample_sps ?? "-"}</p>
              <p><strong>Current Channel:</strong> {data.channel_current ?? data.channel ?? "-"}</p>
              <p><strong>Voltage Channel:</strong> {data.channel_voltage ?? "-"}</p>
              <p><strong>Note:</strong> {data.note ?? "-"}</p>
            </div>
          </>
        )}
      </div>
    </div>
  );
}