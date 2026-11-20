# Tessera Language Support

VS Code syntax highlighting and editor configuration for Tessera (`.t`, `.csec`) files.

It recognizes comments, strings and `${...}` interpolation, numeric literals,
declarations, core types, the `<<|` and `>>|` pipeline operators, and the current
Tessera sugar syntax such as `build` blocks and token-pattern expressions.

## Install locally

1. Open `editors/vscode-tessera` in VS Code.
2. Run **Extensions: Install from VSIX...** after packaging the extension.

To create a VSIX, install the VS Code extension packager once and run:

```powershell
npm install --global @vscode/vsce
cd editors/vscode-tessera
vsce package
```

For extension development, open this folder in VS Code and press `F5` to launch an
Extension Development Host.
