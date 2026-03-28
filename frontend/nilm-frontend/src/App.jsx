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
import AuthPage from "./AuthPage";
import { clearToken, getToken } from "./api";
import feiLogo from "./assets/fei-stu-logo.png";

const API_BASE = "http://127.0.0.1:8001";

function formatNumber(value, digits = 2) {
  if (value === null || value === undefined) return "-";
  return Number(value).toFixed(digits);
}

function formatDateTime(value) {
  if (!value) return "-";
  const d = new Date(value);
  if (Number.isNaN(d.getTime())) return value;
  return d.toLocaleString();
}

function formatTime(value) {
  if (!value) return "-";
  const d = new Date(value);
  if (Number.isNaN(d.getTime())) return value;
  return d.toLocaleTimeString();
}

function MetricCard({ title, value, unit }) {
  return (
    <div
      style={{
        border: "1px solid #e5e7eb",
        borderRadius: 16,
        padding: 20,
        minWidth: 220,
        flex: "1 1 220px",
        background: "#ffffff",
        boxShadow: "0 4px 16px rgba(0,0,0,0.05)",
      }}
    >
      <div style={{ fontSize: 14, color: "#6b7280", marginBottom: 8 }}>
        {title}
      </div>
      <div style={{ fontSize: 32, fontWeight: 700, color: "#111827" }}>
        {value} <span style={{ fontSize: 18, fontWeight: 500 }}>{unit}</span>
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
    .map((row) =>
      row.map((value) => `"${String(value).replace(/"/g, '""')}"`).join(",")
    )
    .join("\n");

  const blob = new Blob([csv], { type: "text/csv;charset=utf-8;" });
  const url = URL.createObjectURL(blob);

  const a = document.createElement("a");
  a.href = url;
  a.download = "measurements_history.csv";
  a.click();

  URL.revokeObjectURL(url);
}

const thStyle = {
  textAlign: "left",
  padding: "14px 12px",
  borderBottom: "1px solid #e5e7eb",
  color: "#374151",
  fontSize: 14,
};

const tdStyle = {
  padding: "14px 12px",
  borderBottom: "1px solid #f3f4f6",
  color: "#111827",
  fontSize: 14,
};

function buttonStyle(active) {
  return {
    border: active ? "none" : "1px solid #d1d5db",
    background: active ? "#111827" : "#ffffff",
    color: active ? "#ffffff" : "#111827",
    padding: "10px 14px",
    borderRadius: 10,
    cursor: "pointer",
    fontSize: 14,
    fontWeight: 600,
  };
}

function smallButtonStyle(active) {
  return {
    border: active ? "none" : "1px solid #d1d5db",
    background: active ? "#111827" : "#ffffff",
    color: active ? "#ffffff" : "#111827",
    padding: "8px 12px",
    borderRadius: 8,
    cursor: "pointer",
    fontSize: 13,
    fontWeight: 600,
  };
}

export default function App() {
  const [isAuthenticated, setIsAuthenticated] = useState(!!getToken());

  const [data, setData] = useState(null);
  const [history, setHistory] = useState([]);
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(true);

  const [historyLimit, setHistoryLimit] = useState(50);
  const [chartMetric, setChartMetric] = useState("power");
  const [backendOnline, setBackendOnline] = useState(false);
  const [lastUpdate, setLastUpdate] = useState("");

  async function load() {
    try {
      const latestRes = await fetch(`${API_BASE}/measurements/latest`);
      if (!latestRes.ok) {
        throw new Error(`Latest API error: ${latestRes.status}`);
      }
      const latest = await latestRes.json();

      const historyRes = await fetch(
        `${API_BASE}/measurements/history?limit=${historyLimit}`
      );
      if (!historyRes.ok) {
        throw new Error(`History API error: ${historyRes.status}`);
      }
      const historyJson = await historyRes.json();

      setData(latest);
      setHistory(historyJson.items ?? []);
      setBackendOnline(true);
      setLastUpdate(new Date().toLocaleTimeString());
      setError("");
    } catch (e) {
      setBackendOnline(false);
      setError(`Failed to load data from backend: ${e.message}`);
    } finally {
      setLoading(false);
    }
  }

  useEffect(() => {
    if (!isAuthenticated) return;

    load();
    const id = setInterval(load, 2000);
    return () => clearInterval(id);
  }, [historyLimit, isAuthenticated]);

  const chartData = useMemo(() => {
    return [...history].reverse().map((item) => ({
      ...item,
      time: formatTime(item.timestamp),
    }));
  }, [history]);

  const chartConfig = {
    power: {
      dataKey: "power",
      label: "Apparent Power (S)",
      unit: "VA",
    },
    current: {
      dataKey: "current",
      label: "Current RMS",
      unit: "A",
    },
    voltage: {
      dataKey: "voltage",
      label: "Voltage RMS",
      unit: "V",
    },
  };

  const currentChart = chartConfig[chartMetric];

  function handleLogout() {
    clearToken();
    setIsAuthenticated(false);
  }

  if (!isAuthenticated) {
    return <AuthPage onLoginSuccess={() => setIsAuthenticated(true)} />;
  }

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
        <div
          style={{
            background: "#1639a5",
            color: "#ffffff",
            borderRadius: 20,
            padding: "20px 24px",
            boxShadow: "0 10px 24px rgba(0,0,0,0.12)",
            display: "flex",
            justifyContent: "space-between",
            alignItems: "center",
            gap: 20,
            flexWrap: "wrap",
          }}
        >
          <div
            style={{
              display: "flex",
              alignItems: "center",
              gap: 18,
              flexWrap: "wrap",
            }}
          >
            <img
              src={feiLogo}
              alt="FEI STU"
              style={{
                height: 64,
                width: "auto",
                borderRadius: 8,
                padding: 4,
              }}
            />

            <div>
              <h1
                style={{
                  margin: 0,
                  fontSize: 34,
                  color: "#ffffff",
                }}
              >
                Power Measurement Dashboard
              </h1>

              <p
                style={{
                  marginTop: 8,
                  marginBottom: 0,
                  color: "rgba(255,255,255,0.88)",
                  fontSize: 15,
                }}
              >
                Live current, voltage and power measurements from ESP32 + ADS1256
              </p>
            </div>
          </div>

          <button
            onClick={handleLogout}
            style={{
              border: "1px solid rgba(255,255,255,0.25)",
              background: "#ffffff",
              color: "#1e40af",
              padding: "10px 16px",
              borderRadius: 12,
              cursor: "pointer",
              fontSize: 14,
              fontWeight: 700,
            }}
          >
            Logout
          </button>
        </div>

        <div
          style={{
            marginTop: 18,
            display: "flex",
            gap: 12,
            flexWrap: "wrap",
            alignItems: "center",
          }}
        >
          <div
            style={{
              padding: "8px 12px",
              borderRadius: 999,
              background: backendOnline ? "#dcfce7" : "#fee2e2",
              color: backendOnline ? "#166534" : "#991b1b",
              fontWeight: 600,
              fontSize: 14,
            }}
          >
            Backend: {backendOnline ? "Online" : "Offline"}
          </div>

          <div
            style={{
              padding: "8px 12px",
              borderRadius: 999,
              background: "#ffffff",
              border: "1px solid #e5e7eb",
              color: "#374151",
              fontSize: 14,
            }}
          >
            Last update: {lastUpdate || "-"}
          </div>
        </div>

        {loading && <p style={{ marginTop: 16 }}>Загрузка...</p>}

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
              <MetricCard
                title="Current RMS"
                value={formatNumber(data.current)}
                unit="A"
              />
              <MetricCard
                title="Voltage RMS"
                value={formatNumber(data.voltage)}
                unit="V"
              />
              <MetricCard
                title="Apparent Power (S)"
                value={formatNumber(data.power)}
                unit="VA"
              />
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

              <div
                style={{
                  display: "grid",
                  gridTemplateColumns: "repeat(auto-fit, minmax(220px, 1fr))",
                  gap: 16,
                  marginTop: 16,
                }}
              >
                <div><strong>Measurement ID:</strong> {data.id ?? "-"}</div>
                <div><strong>Device ID:</strong> {data.device_id ?? "-"}</div>
                <div><strong>Device Name:</strong> {data.device_name ?? "-"}</div>
                <div><strong>Timestamp:</strong> {formatDateTime(data.timestamp)}</div>
                <div><strong>Channel:</strong> {data.channel ?? "-"}</div>
              </div>
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
              marginBottom: 16,
            }}
          >
            <h2 style={{ margin: 0, color: "#111827" }}>History Chart</h2>

            <div style={{ display: "flex", gap: 10, flexWrap: "wrap" }}>
              <button
                onClick={() => setChartMetric("power")}
                style={buttonStyle(chartMetric === "power")}
              >
                Power
              </button>
              <button
                onClick={() => setChartMetric("current")}
                style={buttonStyle(chartMetric === "current")}
              >
                Current
              </button>
              <button
                onClick={() => setChartMetric("voltage")}
                style={buttonStyle(chartMetric === "voltage")}
              >
                Voltage
              </button>

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
                    dataKey={currentChart.dataKey}
                    name={`${currentChart.label} (${currentChart.unit})`}
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
          <div
            style={{
              display: "flex",
              justifyContent: "space-between",
              alignItems: "center",
              gap: 16,
              flexWrap: "wrap",
            }}
          >
            <h2 style={{ marginTop: 0, color: "#111827" }}>Measurement History</h2>

            <div style={{ display: "flex", gap: 8 }}>
              <button onClick={() => setHistoryLimit(20)} style={smallButtonStyle(historyLimit === 20)}>
                20
              </button>
              <button onClick={() => setHistoryLimit(50)} style={smallButtonStyle(historyLimit === 50)}>
                50
              </button>
              <button onClick={() => setHistoryLimit(100)} style={smallButtonStyle(historyLimit === 100)}>
                100
              </button>
            </div>
          </div>

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
                      <td style={tdStyle}>{formatDateTime(item.timestamp)}</td>
                      <td style={tdStyle}>{item.device_id}</td>
                      <td style={tdStyle}>{formatNumber(item.current)}</td>
                      <td style={tdStyle}>{formatNumber(item.voltage)}</td>
                      <td style={tdStyle}>{formatNumber(item.power)}</td>
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