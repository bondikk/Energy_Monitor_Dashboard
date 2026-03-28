import { useEffect, useMemo, useState } from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip,
  ResponsiveContainer,
  CartesianGrid,
  Legend,
} from "recharts";

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
        {value ?? "-"}{" "}
        <span style={{ fontSize: 18, fontWeight: 500 }}>{unit}</span>
      </div>
    </div>
  );
}

function exportHistoryToCSV(history) {
  if (!history || history.length === 0) return;

  const header = [
    "id",
    "timestamp",
    "device_id",
    "device_name",
    "current",
    "voltage",
    "power",
    "channel",
  ];

  const rows = history.map((item) => [
    item.id ?? "",
    item.timestamp ?? "",
    item.device_id ?? "",
    item.device_name ?? "",
    item.current ?? "",
    item.voltage ?? "",
    item.power ?? "",
    item.channel ?? "",
  ]);

  const csv = [header, ...rows]
    .map((row) => row.map((value) => `"${String(value).replace(/"/g, '""')}"`).join(","))
    .join("\n");

  const blob = new Blob([csv], { type: "text/csv;charset=utf-8;" });
  const url = URL.createObjectURL(blob);

  const a = document.createElement("a");
  a.href = url;
  a.download = "measurements_history.csv";
  a.click();

  URL.revokeObjectURL(url);
}

export default function App() {
  const [data, setData] = useState(null);
  const [history, setHistory] = useState([]);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);

  async function load() {
    try {
      const latestRes = await fetch(`${API_BASE}/measurements/latest`);
      if (!latestRes.ok) {
        throw new Error(`Latest API error: ${latestRes.status}`);
      }
      const latest = await latestRes.json();
      setData(latest);

      const historyRes = await fetch(`${API_BASE}/measurements/history?limit=50`);
      if (!historyRes.ok) {
        throw new Error(`History API error: ${historyRes.status}`);
      }
      const historyJson = await historyRes.json();
      setHistory(historyJson.items ?? []);

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

  const chartData = useMemo(() => {
    return [...history]
      .reverse()
      .map((item) => ({
        ...item,
        time: new Date(item.timestamp).toLocaleTimeString(),
      }));
  }, [history]);

  return (
    <div
      style={{
        minHeight: "100vh",
        background: "#f3f4f6",
        padding: 32,
        fontFamily: "Inter, Arial, sans-serif",
      }}
    >
      <div style={{ maxWidth: 1200, margin: "0 auto" }}>
        <h1 style={{ margin: 0, fontSize: 36, color: "#111827" }}>
          NILM Dashboard
        </h1>

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
          <div
            style={{
              display: "flex",
              justifyContent: "space-between",
              alignItems: "center",
              gap: 16,
              flexWrap: "wrap",
            }}
          >
            <h2 style={{ marginTop: 0, marginBottom: 16, color: "#111827" }}>
              Power History
            </h2>

            <button
              onClick={() => exportHistoryToCSV(history)}
              style={{
                border: "none",
                background: "#111827",
                color: "#ffffff",
                padding: "10px 16px",
                borderRadius: 10,
                cursor: "pointer",
                fontSize: 14,
                fontWeight: 600,
              }}
            >
              Export CSV
            </button>
          </div>

          {chartData.length === 0 ? (
            <p style={{ color: "#6b7280" }}>История пока пустая.</p>
          ) : (
            <div style={{ width: "100%", height: 320 }}>
              <ResponsiveContainer>
                <LineChart data={chartData}>
                  <CartesianGrid strokeDasharray="3 3" />
                  <XAxis dataKey="time" />
                  <YAxis />
                  <Tooltip />
                  <Legend />
                  <Line
                    type="monotone"
                    dataKey="power"
                    name="Power (VA)"
                    stroke="#111827"
                    strokeWidth={2}
                    dot={false}
                  />
                </LineChart>
              </ResponsiveContainer>
            </div>
          )}
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
          <h2 style={{ marginTop: 0, color: "#111827" }}>Measurement History</h2>

          {history.length === 0 ? (
            <p style={{ color: "#6b7280" }}>Нет данных истории.</p>
          ) : (
            <div style={{ overflowX: "auto" }}>
              <table
                style={{
                  width: "100%",
                  borderCollapse: "collapse",
                  marginTop: 12,
                }}
              >
                <thead>
                  <tr style={{ background: "#f9fafb" }}>
                    <th style={thStyle}>ID</th>
                    <th style={thStyle}>Time</th>
                    <th style={thStyle}>Device</th>
                    <th style={thStyle}>Current (A)</th>
                    <th style={thStyle}>Voltage (V)</th>
                    <th style={thStyle}>Power (VA)</th>
                    <th style={thStyle}>Channel</th>
                  </tr>
                </thead>
                <tbody>
                  {history.map((item) => (
                    <tr key={item.id}>
                      <td style={tdStyle}>{item.id}</td>
                      <td style={tdStyle}>{item.timestamp}</td>
                      <td style={tdStyle}>{item.device_id}</td>
                      <td style={tdStyle}>{item.current}</td>
                      <td style={tdStyle}>{item.voltage}</td>
                      <td style={tdStyle}>{item.power}</td>
                      <td style={tdStyle}>{item.channel ?? "-"}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

const thStyle = {
  textAlign: "left",
  padding: "12px 10px",
  borderBottom: "1px solid #e5e7eb",
  color: "#374151",
  fontSize: 14,
};

const tdStyle = {
  padding: "12px 10px",
  borderBottom: "1px solid #f3f4f6",
  color: "#111827",
  fontSize: 14,
};