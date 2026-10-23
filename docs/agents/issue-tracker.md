# Issue tracker: Linear

Issues and specs for this repo live in Linear, in the **LegacyCore Lib** project
(`P-INN-2`, https://linear.app/contact-ci/project/legacycore-lib-f7751e7a8e37),
owned by the **Innovobot** team (key `INN`, a sub-team of Devs, so issues are `INN-123`). Use the Linear MCP
tools (`mcp__claude_ai_Linear__*`) for every operation. If they're missing, ask the
user to run `/mcp` and connect "claude.ai Linear".

## Team move (23 Oct 2026)

The Linux SDK projects moved from the Devs team (`DEV`) to its sub-team Innovobot (`INN`) on 23 Oct 2026. Older `DEV-n` identifiers in this repo's docs, commits and PRs still resolve in Linear (for example `DEV-22` opens `INN-51`), so they need no rewriting; new references use `INN-n`. Statuses are the same set on both teams, with one exception: `Triage` has to be enabled on the Innovobot team for the "new issues start in Triage" rule and the triage roles below to work; until it is, new issues land in `Backlog`.

## Conventions

- **Create an issue**: `save_issue` with `team: "INN"`, `project: "LegacyCore Lib"`, `title`,
  and a Markdown `description` (real newlines, not escapes). New issues start in the
  **Triage** status unless a skill says otherwise. Add a type label (`Bug`, `Feature`,
  or `Improvement`) when the type is clear. Add a Platform label (for example
  `addLabels: ["Linux"]`) only when the issue is specific to one platform; see
  "Platform label".
- **Read an issue**: `get_issue` with the identifier (`INN-123`), then `list_comments`
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

## Platform label

The workspace label group **Platform** is single-select, with labels `Windows`, `Linux`
and `Android`. Apply one only when an issue is specific to that platform. No Platform
label means the issue applies everywhere: protocol decisions, Core API changes, CI
standards, cross-platform behaviour.

Platform says where it runs; the Linear project says which repo changes. With the label
set, titles don't need an "on Linux:" prefix. Apply it when creating the issue
(`save_issue` with `addLabels: ["Linux"]`) and when triaging (`save_issue` with `id` and
`addLabels`).

## Repo links

A link from an issue or comment to a file in a repo must outlive the branch it was written on. Use a commit-pinned permalink (`.../blob/<sha>/<path>`, GitHub's `y` shortcut) when the content at that point in time matters, or the path on the default branch (`main`, or `staging` for work not yet released) when the current content matters. Never link to a feature branch: it is deleted when its PR merges and the link dies, which is how the Core-Service contract links in DEV-106 and DEV-107 broke. Content that exists only on an unmerged branch is linked through its PR. When a PR merges, rewrite any branch link in the issue description or completion comment.

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

Create a Linear issue in the LegacyCore Lib project (team `INN`).

## When a skill says "fetch the relevant ticket"

`get_issue` with the identifier, plus `list_comments` on it.

## Wayfinding operations

Used by `/wayfinder`. The **map** is one Linear issue, and its **child** tickets are
its sub-issues.

- **Map**: an issue labelled `wayfinder:map` in the LegacyCore Lib project, holding the
  Notes / Decisions-so-far / Fog body.
- **Child ticket**: a sub-issue of the map (`save_issue` with `parentId: <map>`),
  labelled `wayfinder:<type>` (`research` / `prototype` / `grilling` / `task`).
  Create any missing `wayfinder:*` labels in the INN team on first use.
- **Blocking**: Linear's native relation, set with `save_issue` `blockedBy: [...]`.
  A ticket is unblocked once every blocker is in a completed or canceled state.
- **Frontier query**: `list_issues` with `parentId: <map>`, read each with
  `get_issue` `includeRelations: true`, and drop any that have an open blocker or an
  assignee. The first remaining one in map order wins.
- **Claim**: `save_issue` with `assignee: "me"` and `state: "In Progress"`, as the
  session's first write.
- **Resolve**: `save_comment` with the answer, set `state: "Done"`, then append a
  pointer (gist plus `INN-n` link) to the map's Decisions-so-far.
