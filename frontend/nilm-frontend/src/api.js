const API_BASE = "http://127.0.0.1:8001";

export async function registerUser({ email, username, password }) {
  const res = await fetch(`${API_BASE}/auth/register`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ email, username, password }),
  });

  const data = await safeJson(res);

  if (!res.ok) {
    throw new Error(extractErrorMessage(data, res.status, "Registration failed"));
  }

  return data;
}

export async function loginUser({ email, password }) {
  const res = await fetch(`${API_BASE}/auth/login`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({ email, password }),
  });

  const data = await safeJson(res);

  if (!res.ok) {
    throw new Error(extractErrorMessage(data, res.status, "Login failed"));
  }

  return data;
}

export function saveToken(token) {
  localStorage.setItem("access_token", token);
}

export function getToken() {
  return localStorage.getItem("access_token");
}

export function clearToken() {
  localStorage.removeItem("access_token");
}

async function safeJson(res) {
  try {
    return await res.json();
  } catch {
    return null;
  }
}

function extractErrorMessage(data, status, fallback) {
  if (!data) return `${fallback} (${status})`;
  if (typeof data.detail === "string") return data.detail;
  if (Array.isArray(data.detail)) return data.detail.map((x) => x.msg).join(", ");
  if (data.message) return data.message;
  return `${fallback} (${status})`;
}