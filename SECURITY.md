# Security Policy

## Reporting a Vulnerability

To report a security vulnerability, please use [GitHub Security Advisories](https://github.com/straplocked/advanced-mb/security/advisories/new) for private reporting.

Do **not** open a public issue for security vulnerabilities.

## Scope

Advanced MB is an audio plugin that runs locally within a DAW host. It has **no network access** (`JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`).

Potential security concerns are limited to:

- **Preset file parsing** -- malformed preset files could potentially cause crashes or unexpected behavior
- **Buffer handling** -- edge cases in audio buffer processing

## Supported Versions

| Version | Supported |
|---------|-----------|
| 0.1.x   | Yes       |
