# Integration branch

This branch (`dev/integration`) is for cross-pollinating work between
Dadud/Motor-City-Online-RE and americusmaximus/AZMCO:

- **mco-re**: your RE work — extraction tools, format docs, gameplay data, custom local shard server, mcity disassembly research
- **azmco**: full source ports of the render engine

Modernization roadmap:
1. Use mco-re's format docs to extract MCO assets cleanly
2. Use mco-re's data/*.csv as ground truth for asset verification
3. Plug mco-re's render.c insights into AZMCO's R.DirectX.8.0.A as needed
4. The dx8z reset-loop 2-byte patch (in ~/Apps/MCOHackAnalysis/) maps to AZMCO's R.DirectX.8.0.A source

Companion workspace: ~/projects/azmco-dev/
