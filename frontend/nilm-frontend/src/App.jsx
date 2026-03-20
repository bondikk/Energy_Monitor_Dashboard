import { useEffect, useState } from "react";

const API_BASE = "http://127.0.0.1:8001";

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
      <div style={{ fontSize: 14, color: "#6b7280", marginBottom: 8 }}>
        {title}
      </div>
      <div style={{ fontSize: 32, fontWeight: 700, color: "#111827" }}>
        {value ?? "-"} <span style={{ fontSize: 18, fontWeight: 500 }}>{unit}</span>
      </div>
    </div>
  );
}

export default function App() {
  const [data, setData] = useState(null);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);

  async function load() {
    try {
      const res = await fetch(`${API_BASE}/measurements/latest`);
      if (!res.ok) {
        throw new Error(`API error: ${res.status}`);
      }
      const result = await res.json();
      setData(result);
      setError("");
    } catch (e) {
  setError(`Не удалось получить данные с backend: ${e.message}`);
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
              <MetricCard title="Current" value={data.current} unit="A" />
              <MetricCard title="Voltage" value={data.voltage} unit="V" />
              <MetricCard title="Apparent Power" value={data.power} unit="VA" />
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
              <p><strong>Measurement ID:</strong> {data.id ?? "-"}</p>
              <p><strong>Device ID:</strong> {data.device_id ?? "-"}</p>
              <p><strong>Device Name:</strong> {data.device_name ?? "-"}</p>
              <p><strong>Timestamp:</strong> {data.timestamp ?? "-"}</p>
              <p><strong>Channel:</strong> {data.channel ?? "-"}</p>
            </div>
          </>
        )}
      </div>
    </div>
  );
}