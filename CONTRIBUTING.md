# Contributing to FluxMirror

Thank you for your interest in contributing to **FluxMirror**! We welcome bug fixes, documentation improvements, new features, and feedback.

---

## 🛠️ Development Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/milan-m-antony/FluxMirror.git
   cd FluxMirror
   ```

2. **Install dependencies:**
   ```bash
   npm install
   ```

3. **Start the local development server:**
   ```bash
   npm run dev
   ```

4. **Connect an Android device with USB Debugging enabled:**
   Verify with:
   ```bash
   adb devices
   ```

5. **Test production builds before opening a PR:**
   ```bash
   npm run build
   ```

---

## 📋 Pull Request Process

1. Fork the repo and create your branch from `main`:
   ```bash
   git checkout -b feat/your-feature-name
   ```
2. Keep commit messages clear following [Conventional Commits](https://www.conventionalcommits.org/) (`feat:`, `fix:`, `docs:`, `refactor:`).
3. Ensure no compile or TypeScript errors remain:
   ```bash
   npm run build
   ```
4. Open a Pull Request on GitHub with a description of your changes and any relevant screenshots.

---

## 💬 Community & Feedback
Feel free to open an [Issue](https://github.com/milan-m-antony/FluxMirror/issues) for feature requests or bug reports!
