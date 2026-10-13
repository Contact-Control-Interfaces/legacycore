# Triage Labels

The skills speak in terms of five canonical triage roles. In this repo's Linear team
(`DEV`), most of them are **issue statuses**, not labels.

| Role in mattpocock/skills | In our Linear                              | Meaning                                  |
| ------------------------- | ------------------------------------------ | ---------------------------------------- |
| `needs-triage`            | status **Triage**                          | Maintainer needs to evaluate this issue  |
| `needs-info`              | label **Needs Info** (status stays Triage) | Waiting on reporter for more information |
| `ready-for-agent`         | status **Todo**                            | Fully specified, ready for an AFK agent  |
| `ready-for-human`         | status **Needs Human**                     | Requires human implementation            |
| `wontfix`                 | status **Wont Fix**                        | Will not be actioned                     |

Applying a status role means `save_issue` with `state: "<status>"`. Applying
`needs-info` means `addLabels: ["Needs Info"]` while leaving the issue in Triage, and
posting the open questions as a comment. Remove the label once the reporter answers.
If the `Needs Info` label doesn't exist yet, create it in the DEV team.

The `Bug` / `Feature` / `Improvement` labels describe the issue's type, not its triage
state. Leave them alone when moving an issue between triage roles.
