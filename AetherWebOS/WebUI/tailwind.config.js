/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        aether: {
          black: '#050505',    // Background
          surface: '#121212',  // Window/Card
          cyan: '#00f2ff',     // Link UP / Active
          red: '#ff0033',      // Alert / Shutdown
          gray: '#888888',     // Subtext
        }
      },
      fontFamily: {
        mono: ['JetBrains Mono', 'Fira Code', 'monospace'],
      },
    },
  },
  plugins: [],
}