# PrecisionPour - Business & Product Discovery Questions (Fill-In)

Use this document to align on product intent, constraints, and decisions. Keep answers short and concrete; link out to external docs if needed.

## Document Control

- **Date**:
- **Attendees**:
- **Owner**:
- **Status**: Draft / Agreed / Superseded
- **Scope**: Events / Pubs / Home prototype / Other:

## 1) Summary (1 paragraph)

- **What we’re building**:
- **Who it’s for**:
- **Why now / why us**:

## 2) Customer & Setting

### Target customers (ranked)

- **Primary (near-term wedge)**:
- **Secondary**:
- **Future**:

### Operating environment constraints

- **Cleaning / hygiene**:
- **Temperature range**:
- **Viscosity / drink types**:
- **Install constraints** (gas lines, keg/cask compatibility, footprint, power, network):
- **Throughput expectations** (pours per hour per tap):

### Success criteria (from the customer’s view)

- **Quality of service**:
- **Speed of service**:
- **Reliability / uptime expectations**:
- **Ease of use** (first-time user, accessibility):

## 3) “Precision” Definition (What must be accurate)

### Commercial unit of account

- **Charging model**: per ml / per pint / per time / other:
- **Supported serve sizes**: half pint / pint / custom:

### Accuracy requirements

- **Target accuracy** (e.g., ±X ml or X%):
- **Repeatability** (pour-to-pour variance):
- **Edge cases** (foam, partial pours, stop/start behavior):
- **Dispute policy** (what happens if user claims under-dispense?):

## 4) System Boundary & Hardware Roadmap

### Current prototype

- **Prototype platform**: PerfectDraft + ESP32 (touch UI + flow sensor + solenoid)
- **What we’re validating now** (demo goals / learning goals):

### Intended production direction

- **Custom hardware plan** (likely NXP-based):
- **What must change for events** (ruggedization, install, serviceability, security, cost):
- **One-tap-per-device vs multi-tap**:
- **Any expected “hub” component**:

## 5) Connectivity, Backend, and Integrations

### Connectivity goals (what actually matters)

- **Primary purpose**: authorization / telemetry / remote config / command-control / other:
- **Preferred transport**: MQTT (preferred) / HTTPS / other:
- **Who operates infra**: we manage broker + portal + payments

### Third-party systems to integrate with (research list)

- **Hospitality POS** (examples/targets):
- **Event ticketing/wristbands**:
- **Payments**:
- **Venue ops tools**:

### Data & reporting needs

- **Operator dashboard must-have**:
- **Reconciliation** (money in vs ml out):
- **Audit trail expectations**:

## 6) Onboarding & Payment Flow

### Intended end-user flow

- Scan QR → web portal page (device-specific) → user logs in / creates account → buys tokens/credit → device authorized → user pours → credit decremented by measured ml → valve closes on zero credit or end of pour

### QR code usage

- **Primary**: payment entry point (device-specific URL / identifier)
- **Secondary**: onboarding / support / device info (optional)

### Provisioning/onboarding for operators

- **Preferred**: Improv BLE via custom mobile app
- **Fallback** (if BLE unavailable):

## 7) Reliability, Safety, and Fail-Safes

### Availability expectations

- **Event scenario uptime target**:
- **Max acceptable “time to recover”** if a tap fails:

### Safety rules (define the hard invariants)

- **Valve fail state on power loss**:
- **Watchdog/reset behavior**:
- **Stuck-open detection/mitigation**:
- **What happens on network drop mid-pour**:

## 8) Fraud / Tamper / Threat Model (Practical)

### What we must defend against

- **QR label copying / swapping**:
- **Replay of “enable” messages**:
- **Device impersonation**:
- **Physical bypass** (valve, flow sensor, handle switch):
- **Account sharing / resale of tokens**:

### Security decisions

- **Device identity** (how device is uniquely identified/provisioned):
- **AuthN/AuthZ** between portal and device:
- **Signing/encryption requirements**:
- **Key management** (factory vs field provisioning):

## 9) Calibration & Drift

### Calibration plan

- **Factory calibration**:
- **On-site calibration**:
- **How often**:
- **Who performs it**:

### Evidence & auditability

- **How calibration is recorded**:
- **What the operator can verify**:

## 10) Compliance / Regulatory (Decisions Needed)

### Legal metrology / “weights & measures”

- **Do we claim exact delivered volume?** yes/no
- **If yes**: required approvals/certifications and timeline:
- **If no**: how we phrase the value proposition and billing basis:

### Food safety / materials / cleaning compliance

- Requirements:

## 11) Open Questions & Next Experiments

Add the next smallest experiment that reduces risk.

- **Q**:
  - **Why it matters**:
  - **Proposed experiment**:
  - **Owner**:
  - **Due**:

