import { useState } from "react";
import { loginUser, registerUser, saveToken } from "./api";

export default function AuthPage({ onLoginSuccess }) {
  const [registerForm, setRegisterForm] = useState({
    email: "",
    username: "",
    password: "",
  });

  const [loginForm, setLoginForm] = useState({
    email: "",
    password: "",
  });

  const [registerMessage, setRegisterMessage] = useState("");
  const [loginMessage, setLoginMessage] = useState("");
  const [loadingRegister, setLoadingRegister] = useState(false);
  const [loadingLogin, setLoadingLogin] = useState(false);

  async function handleRegister(e) {
    e.preventDefault();
    setRegisterMessage("");
    setLoadingRegister(true);

    try {
      await registerUser(registerForm);
      setRegisterMessage("Registration successful. Now log in.");
      setRegisterForm({ email: "", username: "", password: "" });
    } catch (e) {
      setRegisterMessage(`Registration error: ${e.message}`);
    } finally {
      setLoadingRegister(false);
    }
  }

  async function handleLogin(e) {
    e.preventDefault();
    setLoginMessage("");
    setLoadingLogin(true);

    try {
      const data = await loginUser(loginForm);

      const token =
        data?.access_token ||
        data?.token ||
        data?.jwt ||
        null;

      if (!token) {
        throw new Error("Token not found in login response");
      }

      saveToken(token);
      setLoginMessage("Login successful");
      onLoginSuccess();
    } catch (e) {
      setLoginMessage(`Login error: ${e.message}`);
    } finally {
      setLoadingLogin(false);
    }
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
      <div style={{ maxWidth: 1100, margin: "0 auto" }}>
        <h1 style={{ margin: 0, fontSize: 36, color: "#111827" }}>
        Measurement Dashboard Access
        </h1>
        <p style={{ color: "#6b7280", marginTop: 8 }}>
            Sign in to access live electrical measurements
        </p>

        <div
          style={{
            display: "grid",
            gridTemplateColumns: "repeat(auto-fit, minmax(320px, 1fr))",
            gap: 24,
            marginTop: 28,
          }}
        >
          <div style={cardStyle}>
            <h2 style={titleStyle}>Register</h2>

            <form onSubmit={handleRegister}>
              <label style={labelStyle}>Email</label>
              <input
                style={inputStyle}
                type="email"
                value={registerForm.email}
                onChange={(e) =>
                  setRegisterForm({ ...registerForm, email: e.target.value })
                }
                required
              />

              <label style={labelStyle}>Username</label>
              <input
                style={inputStyle}
                type="text"
                value={registerForm.username}
                onChange={(e) =>
                  setRegisterForm({ ...registerForm, username: e.target.value })
                }
                required
              />

              <label style={labelStyle}>Password</label>
              <input
                style={inputStyle}
                type="password"
                value={registerForm.password}
                onChange={(e) =>
                  setRegisterForm({ ...registerForm, password: e.target.value })
                }
                required
              />

              <button style={buttonStyle} type="submit" disabled={loadingRegister}>
                {loadingRegister ? "Registering..." : "Register"}
              </button>
            </form>

            {registerMessage && <p style={messageStyle}>{registerMessage}</p>}
          </div>

          <div style={cardStyle}>
            <h2 style={titleStyle}>Login</h2>

            <form onSubmit={handleLogin}>
              <label style={labelStyle}>Email</label>
              <input
                style={inputStyle}
                type="email"
                value={loginForm.email}
                onChange={(e) =>
                  setLoginForm({ ...loginForm, email: e.target.value })
                }
                required
              />

              <label style={labelStyle}>Password</label>
              <input
                style={inputStyle}
                type="password"
                value={loginForm.password}
                onChange={(e) =>
                  setLoginForm({ ...loginForm, password: e.target.value })
                }
                required
              />

              <button style={buttonStyle} type="submit" disabled={loadingLogin}>
                {loadingLogin ? "Logging in..." : "Login"}
              </button>
            </form>

            {loginMessage && <p style={messageStyle}>{loginMessage}</p>}
          </div>
        </div>
      </div>
    </div>
  );
}

const cardStyle = {
  background: "#ffffff",
  border: "1px solid #e5e7eb",
  borderRadius: 16,
  padding: 24,
  boxShadow: "0 4px 16px rgba(0,0,0,0.05)",
};

const titleStyle = {
  marginTop: 0,
  color: "#111827",
};

const labelStyle = {
  display: "block",
  marginTop: 12,
  marginBottom: 6,
  color: "#374151",
  fontWeight: 600,
};

const inputStyle = {
  width: "100%",
  padding: "12px 14px",
  borderRadius: 10,
  border: "1px solid #d1d5db",
  outline: "none",
  fontSize: 14,
  boxSizing: "border-box",
};

const buttonStyle = {
  marginTop: 18,
  border: "none",
  background: "#111827",
  color: "#ffffff",
  padding: "12px 16px",
  borderRadius: 10,
  cursor: "pointer",
  fontSize: 14,
  fontWeight: 600,
  width: "100%",
};

const messageStyle = {
  marginTop: 14,
  color: "#374151",
  whiteSpace: "pre-wrap",
};
