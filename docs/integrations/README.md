# Third-Party Integration Guide Index

Last reviewed: 2026-08-20

Everward is proprietary commercial software in a public repository. External creative, automation, and AI-development tools must therefore be integrated with strict IP, licensing, provenance, safety, and replaceability boundaries.

## Governing rules

1. Unreal Engine remains the game/runtime authority. External tools do not become runtime authorities unless separately approved.
2. Prefer offline/local authoring and development-tool boundaries over embedding third-party systems into proprietary game code.
3. Every generated or AI-assisted shipped asset must have provenance recorded under the rules in `docs/IP_AND_LICENSES.md`.
4. Third-party model/plugin licenses are independent of application licenses and must be checked separately.
5. Generated content is never assumed commercially safe solely because generation succeeded.
6. Agent/development tools may not bypass source control, CI, review, or release gates.
7. Keep canonical save/world/probe state Everward-owned so plugins and planners remain replaceable.
8. Do not copy third-party source into Everward merely to simplify setup; use adapters/plugins/service boundaries where practical.

## Planned integrations

| Project | Role | Default boundary | Status |
|---|---|---|---|
| ComfyUI | Local generative image/video workflow backend | External local service/process | Evaluate for asset ideation/production |
| Krita AI Diffusion | Artist-in-the-loop image editing frontend | External authoring application/plugin | Evaluate |
| ViMax | Agentic storyboard/video production reference and optional marketing pipeline | External tool/reference architecture | Evaluate |
| Remotion | Programmable trailer/devlog/video compositor | Separate media-tool project/process | Evaluate |
| SUSS | Utility AI scoring/execution candidate for probe behavior | Unreal plugin behind Everward-owned behavior/state contracts | High-priority evaluation |
| UE-MCP / Monolith / Hayba | AI-assisted Unreal development tooling | External/editor development interface | Compare; choose one |
| SimWorld | World-observation/action separation reference | Reference architecture | Study |

## Separation of concerns

Creative/media tools support assets and marketing. Utility AI may support runtime decisions behind Everward-owned contracts. Unreal-agent tooling exists only for development and must never become packaged gameplay authority.
