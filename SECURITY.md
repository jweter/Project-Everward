# Security Policy

Everward is currently a private pre-production commercial project.

## Reporting a security issue

Do not disclose suspected vulnerabilities, exposed credentials, proprietary source, or security-sensitive project information publicly.

For now, report security concerns directly to the repository owner through a private channel. If the project later gains external collaborators, testers, services, or a public release, this policy must be updated with a dedicated security contact and coordinated disclosure process before public distribution.

## Secrets

Credentials, API keys, signing keys, store credentials, private certificates, tokens, crash-reporting secrets, and machine-specific environment files must never be committed to the repository.

Use local environment configuration or the relevant platform secret store. `.gitignore` is a convenience layer, not a substitute for secret-management discipline.

If a secret is committed, treat it as compromised: revoke/rotate it first, then remove it from active source control and assess whether history remediation is required.

## Dependencies and supply chain

Third-party code and tools require known provenance and license terms. Security-sensitive dependencies should be pinned or otherwise reproducible where practical, and dependency updates should be reviewed rather than accepted blindly.

## Generated and downloaded content

Do not execute untrusted downloaded scripts, plugins, editor extensions, engine packages, or binary assets solely because they are referenced by project documentation or community content.

## Release security

Before any external alpha, demo, Early Access build, or commercial release, establish at minimum:

- a release-signing strategy where applicable,
- dependency and vulnerability scanning,
- secret scanning,
- provenance for shipped third-party components,
- a vulnerability-reporting route,
- verification that debug/test credentials and developer endpoints are absent from release builds.
