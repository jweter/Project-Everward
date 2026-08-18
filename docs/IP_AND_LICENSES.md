# IP and Licenses

Everward is intended as a future commercial game. Intellectual-property provenance and third-party licensing must therefore be tracked from the beginning rather than reconstructed before release.

## Creative separation

Everward may draw inspiration from broad science-fiction ideas such as self-replicating machines, interstellar exploration, delayed communication, autonomous descendants, and machine civilization. The project must build its own expression, including universe, terminology, characters, visual identity, narrative structure, mechanics, names, UI, artwork, audio, and code.

Do not use franchise-specific material from inspirational works, including protected characters, plot expression, quotes, artwork, logos, or distinctive invented terminology.

In particular, do not use Bobiverse-specific identity or market Everward as an unofficial Bobiverse game.

## Public title clearance

`Everward` is the current working/project title. Before a public commercial launch or substantial marketing commitment, perform a proper name-clearance pass covering relevant trademark databases, game storefronts, web presence, and other commercial uses. A serious release should receive professional IP review where appropriate.

## Asset provenance rule

Every third-party item added to the repository or production project must have recorded provenance and license terms.

Track at minimum:

| Item | Source/creator | License | Commercial use | Modification allowed | Attribution required | Redistribution limits | Repository path | Notes |
|---|---|---|---|---|---|---|---|---|
| Example only | — | — | — | — | — | — | — | Replace before use |

Relevant categories include:

- code dependencies,
- engine plugins,
- shaders,
- fonts,
- icons,
- textures,
- models,
- animations,
- sound effects,
- music,
- reference datasets,
- astronomical catalog data,
- generated assets,
- AI-assisted assets where used,
- build/runtime libraries.

## Engine licenses

Engine choice remains open. Record the exact production-engine version and applicable license terms after the engine decision. Do not assume that an engine's license automatically covers third-party assets or plugins distributed with it.

## Repository visibility and license posture

The repository is intentionally **public** for operational reasons while Everward remains **proprietary commercial software and creative work**.

Public visibility is not an open-source license grant. Viewing, cloning, or otherwise accessing a public repository does not by itself grant permission to copy, modify, redistribute, sublicense, sell, commercialize, or create derivative works from Everward's original materials.

The root `LICENSE` reserves rights rather than granting an open-source license unless an explicit future decision changes that status. Keep that notice prominent and do not add an incompatible open-source project license covering Everward's original work.

Individual third-party components retain their own licenses and must be documented here, in `THIRD_PARTY_NOTICES.md`, or in a dedicated dependency/asset manifest. Their terms may grant rights independently for those specific components and do not alter ownership of Everward's original material.

Because the repository is public:

- never commit secrets, credentials, private keys, store credentials, signing material, or private personal data;
- never commit third-party material that cannot legally be redistributed publicly;
- keep private/generated source material out of the repository where its terms or provenance require that;
- review external contributions for IP provenance before acceptance;
- do not imply that public source availability authorizes public contribution, redistribution, or derivative commercial use.

## AI-assisted content

If generative tools are used for concept art, music, code assistance, text, voice, or other assets, record:

- tool/service,
- date,
- relevant terms or plan,
- human modifications,
- source material supplied to the tool,
- final asset path,
- whether the generated output is shipped or only used as reference.

Do not knowingly feed unlicensed proprietary source assets into generation workflows for commercial derivative use.

## Release gate

Before any public demo, store page, Early Access release, or 1.0 release:

1. audit every shipped dependency and asset,
2. confirm commercial-use rights,
3. satisfy attribution requirements,
4. remove unresolved or provenance-unknown assets,
5. archive required license notices,
6. review the public title and branding,
7. document engine and middleware obligations.
