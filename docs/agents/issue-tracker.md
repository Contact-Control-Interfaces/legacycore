# Issue tracker: Linear

Issues and specs for this repo live in Linear, in the **LegacyCore Lib** project
(`P-DEV-2`, https://linear.app/contact-ci/project/legacycore-lib-f7751e7a8e37),
owned by the **Devs** team (key `DEV`, so issues are `DEV-123`). Use the Linear MCP
tools (`mcp__claude_ai_Linear__*`) for every operation. If they're missing, ask the
user to run `/mcp` and connect "claude.ai Linear".

## Conventions

- **Create an issue**: `save_issue` with `team: "DEV"`, `project: "LegacyCore Lib"`, `title`,
  and a Markdown `description` (real newlines, not escapes). New issues start in the
  **Triage** status unless a skill says otherwise. Add a type label (`Bug`, `Feature`,
  or `Improvement`) when the type is clear.
- **Read an issue**: `get_issue` with the identifier (`DEV-123`), then `list_comments`
  for the discussion.
- **List issues**: `list_issues` with `project: "LegacyCore Lib"`, filtered by `state`
  and/or `label`.
- **Comment**: `save_comment` with `issueId` and `body`.
- **Change status / labels**: `save_issue` with `id` and `state`, or with
  `addLabels` / `removeLabels`. Don't pass `labels` (it replaces the whole set).
- **Close**: post the explanation with `save_comment` first, then set `state` to
  `Done`, `Wont Fix`, `Canceled`, or `Duplicate` (with `duplicateOf`).

Every issue a skill creates for this repo goes in the LegacyCore Lib project. If it
belongs to another repo, use that repo's project (see "Which project"). Never leave
the project unset.

## Which project

Linux work is split by repository into Linear projects under the **Linux SDK** initiative. File each issue in the project for the repo it changes:

| Repo or area | Linear project |
| -- | -- |
| `linux-service` | Linux Service |
| `legacycore` (Core) | LegacyCore Lib |
| `manager` | Manager |
| `communication`, wire recorder and loopback tooling | Glove Protocol & Wire Verification |
| Python binding, ROS 2 packages | Robot Bindings (Python, ROS 2) |
| `fw-*` firmware | Microcontroller Firmware |
| Spans several repos, or a program-level decision | Linux SDK |

## Pull requests as a triage surface

**PRs as a request surface: no.** _(Code is hosted on GitHub at
`Contact-Control-Interfaces/legacycore`; set to `yes` only if external PRs there
should be triaged like issues.)_

## When a skill says "publish to the issue tracker"

Create a Linear issue in the LegacyCore Lib project (team `DEV`).

## When a skill says "fetch the relevant ticket"

`get_issue` with the identifier, plus `list_comments` on it.

## Wayfinding operations

Used by `/wayfinder`. The **map** is one Linear issue, and its **child** tickets are
its sub-issues.

- **Map**: an issue labelled `wayfinder:map` in the LegacyCore Lib project, holding the
  Notes / Decisions-so-far / Fog body.
- **Child ticket**: a sub-issue of the map (`save_issue` with `parentId: <map>`),
  labelled `wayfinder:<type>` (`research` / `prototype` / `grilling` / `task`).
  Create any missing `wayfinder:*` labels in the DEV team on first use.
- **Blocking**: Linear's native relation, set with `save_issue` `blockedBy: [...]`.
  A ticket is unblocked once every blocker is in a completed or canceled state.
- **Frontier query**: `list_issues` with `parentId: <map>`, read each with
  `get_issue` `includeRelations: true`, and drop any that have an open blocker or an
  assignee. The first remaining one in map order wins.
- **Claim**: `save_issue` with `assignee: "me"` and `state: "In Progress"`, as the
  session's first write.
- **Resolve**: `save_comment` with the answer, set `state: "Done"`, then append a
  pointer (gist plus `DEV-n` link) to the map's Decisions-so-far.
