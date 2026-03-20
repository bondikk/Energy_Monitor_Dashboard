const API_BASE = "http://127.0.0.1:8001";

export async function getLatestMeasurement() {
  const res = await fetch(`${API_BASE}/measurements/latest`);
  if (!res.ok) {
    throw new Error(`API error: ${res.status}`);
  }
  return res.json();
}