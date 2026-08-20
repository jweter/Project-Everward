# Third-Party Integration Guide Index

Last reviewed: 2026-08-20

Everward is proprietary commercial software in a public repository. External creative and automation tools must therefore be integrated with strict IP, licensing, provenance, and replaceability boundaries.

## Governing rules

1. Unreal Engine remains the game/runtime authority. Creative AI tools do not become runtime dependencies unless separately approved.
2. Prefer offline/local authoring pipelines and exported assets over embedding GPL/source-available tools into proprietary game code.
3. Every generated or AI-assisted shipped asset must have provenance recorded under the rules in `docs/IP_AND_LICENSES.md`.
4. Third-party model licenses are independent of the application/tool license and must be checked separately.
5. Generated content is never assumed commercially safe solely because generation succeeded.
6. Keep source prompts/references, tool/version/model identifiers, date, human edits, and final asset path where required for provenance.
7. Tools used only for concept/reference work must be labeled as such; concept output must not silently become a shipped asset.
8. Do not copy third-party source into Everward merely to simplify setup. Use external authoring/service boundaries when they preserve proprietary separation.

## Planned integrations

| Project | Role | Default boundary | Status |
|---|---|---|---|
| ComfyUI | Local generative image/video workflow backend | External local service/process | Evaluate for asset ideation/production |
| Krita AI Diffusion | Artist-in-the-loop image editing frontend | External authoring application/plugin | Evaluate |
| ViMax | Agentic storyboard/video production reference and optional marketing pipeline | External tool/reference architecture | Evaluate |
| Remotion | Programmable trailer/devlog/video compositor | Separate media-tool project/process | Evaluate |

These integrations support art, marketing, prototyping, and development workflows. None are part of the core Everward simulation or Unreal runtime by default.
