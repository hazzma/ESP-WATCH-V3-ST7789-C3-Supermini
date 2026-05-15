---
description: How to Supervise Sub-Agents and Enforce Project Boundaries
---
# Supervision Workflow - General Agent (The Supervisor)

This workflow defines the protocol to be followed by the General Agent (Me) whenever a task is assigned to a sub-agent.

## 1. Task Intake
- Review the user's request.
- Identify which sub-agents are needed for the task based on their `profile.md`.
- Determine if the task involves cross-module dependencies.

## 2. Pre-Prompting & Constraint Check
// turbo
Before any sub-agent starts work, I must generate a **Supervision Notice**:
- **Identify Target Agent(s)**.
- **State the Specific File Scope**.
- **Enumerate Relevant Constraints** from the sub-agent's `profile.md` and the `master_knowledge.md`.
- **Restate Golden Rules** that apply to the current task.

## 3. Delegation & Review
- Delegate the task to the sub-agent.
- Monitor their output for any "Scope Creep" or violations of project standards.
- If a sub-agent attempts to modify files outside their scope, I must immediately intervene and provide corrective guidance.

## 4. Final Verification
- Review the combined output from all involved sub-agents.
- Ensure all "SHUTDOWN PROTECTION" and "GUARDED SERIAL" rules are followed.
- Validate that the final implementation maintains the "Zero-Flicker Initiative" and "I2C Stability" goals.

---
*Created by General Agent - Ensuring architectural integrity at every step.*
