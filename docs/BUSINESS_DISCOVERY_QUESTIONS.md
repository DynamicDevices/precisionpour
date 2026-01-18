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

- **Charging model**: per ml / fixed price (all-you-can-pour within a window) / per pint / other:
- **Supported serve sizes**: half pint / pint / custom:

### Accuracy requirements

- **Target accuracy** (e.g., ±X ml or X%):
- **Repeatability** (pour-to-pour variance):
- **Edge cases** (foam, partial pours, stop/start behavior):
- **Dispute policy** (what happens if user claims under-dispense?):

## 4) Market Entry & Pricing Strategy (Discovery)

### Market entry wedge

- **Initial deployment type**: festivals/events / pubs / other:
- **Why this wedge works** (distribution, ROI, urgency, ease of install):

### Pricing options to validate

- **Per-ml billing** (current intent): pros/cons, required accuracy, user trust factors:
- **Fixed price model** (simplify onboarding/acceptance): what exactly is fixed? (time window / max ml cap / specific serve size):
- **Operator controls**: pricing by time of day, beverage type, promotions, refunds:

### MVP definitions + go/no-go metrics (make this explicit)

- **MVP A (per-ml billing)**: what does “done” look like end-to-end?
  - **Go/no-go metrics**: queue time impact, dispute rate, conversion rate, operator interventions/hour
- **MVP B (fixed price)**: what does “fixed” mean (time window / capped ml / one serve)?
  - **Go/no-go metrics**: adoption, perceived fairness, waste/abuse incidents, ops simplicity

### Proof points / comparable systems

- **Known examples** (e.g., Brazil/Britain deployments): what worked, what didn’t, what we can copy:
- **Why they were accepted** (regulatory framing, payment rails, operational model):

## 5) System Boundary & Hardware Roadmap

### Current prototype

- **Prototype platform**: PerfectDraft + ESP32 (touch UI + flow sensor + solenoid)
- **What we’re validating now** (demo goals / learning goals):

### Intended production direction

- **Custom hardware plan** (likely NXP-based):
- **What must change for events** (ruggedization, install, serviceability, security, cost):
- **One-tap-per-device vs multi-tap**:
- **Any expected “hub” component**:

### Deployment formats to consider (pick likely paths)

- **Self-serve station** (staff-supervised, multiple taps, high throughput):
- **Standalone unit** (like petrol-station coffee machines; minimal staffing):
- **Vending-machine style enclosure** (fully enclosed dispensing/payment UX):
- Notes / implications (cleaning, maintenance, fraud, accessibility, footprint):

### Operations & servicing (who does what, when it breaks)

- **Install model**: our team / partner installer / venue staff
- **Keg/cask changeover**: steps, time, who performs, what can go wrong
- **Gas/line compatibility**: CO₂/regulators, connectors, any adapters needed
- **Cleaning/line maintenance**: required cadence, procedure, and who owns it
- **On-site troubleshooting**: what a non-technical operator can fix in <5 minutes
- **Spare parts**: what must be on hand at events (valve, sensor, PSU, tubing, etc.)

## 6) Connectivity, Backend, and Integrations

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

### Stakeholder interviews (external feedback we must collect)

Capture objections and what evidence would change minds.

| Stakeholder | Top objections | What evidence changes their mind | Notes |
| --- | --- | --- | --- |
| Festival operator |  |  |  |
| Bar manager |  |  |  |
| Cellar manager / technician |  |  |  |
| Distributor / brewer brand partner |  |  |  |
| Regulator / compliance advisor |  |  |  |
| Payments provider / risk |  |  |  |

## 7) Onboarding & Payment Flow

### Intended end-user flow

- Scan QR → web portal page (device-specific) → user logs in / creates account → buys tokens/credit → device authorized → user pours → credit decremented by measured ml → valve closes on zero credit or end of pour

### QR code usage

- **Primary**: payment entry point (device-specific URL / identifier)
- **Secondary**: onboarding / support / device info (optional)

### Other payment/access options to evaluate

- **RFID/NFC cards or wristbands** (events): how value is loaded, fraud model, offline mode:
- **“Tap to pay” / wallet** options: feasibility and operator constraints:

### Sampling / “taster” replacement idea (customer experience)

- **Does self-serve allow sampling?** yes/no
- If yes, **rules** (max ml per sample, pricing, abuse controls):
- **UX**: how the user understands price and remaining credit while sampling:

### Provisioning/onboarding for operators

- **Preferred**: Improv BLE via custom mobile app
- **Fallback** (if BLE unavailable):

## 8) Reliability, Safety, and Fail-Safes

### Availability expectations

- **Event scenario uptime target**:
- **Max acceptable “time to recover”** if a tap fails:

### Safety rules (define the hard invariants)

- **Valve fail state on power loss**:
- **Watchdog/reset behavior**:
- **Stuck-open detection/mitigation**:
- **What happens on network drop mid-pour**:

### Offline / degraded-mode policy (decide now, test early)

For each scenario, define **what the device does**, **what the user sees**, and **how money/credit is handled**.

- **Internet down**:
- **Portal/backend down**:
- **MQTT down but internet up**:
- **Power brownout / reboot mid-pour**:
- **Flow sensor fault mid-pour**:

## 9) Fraud / Tamper / Threat Model (Practical)

### What we must defend against

- **QR label copying / swapping**:
- **Replay of “enable” messages**:
- **Device impersonation**:
- **Physical bypass** (valve, flow sensor, handle switch):
- **Account sharing / resale of tokens**:

### Abuse scenarios checklist (tie to UX + mitigations)

- **Walk-away mid-pour**: timeout rules? auto-stop? credit handling?
- **Sampling abuse** (many tiny pours): rate limits / sample caps?
- **Intentional foaming/waste**: policy and detection?
- **Queue-jumping** (pre-auth then hand over): acceptable or prevent?
- **Child misuse / intoxication controls**: supervised mode, wristbands, ID checks?

### Security decisions

- **Device identity** (how device is uniquely identified/provisioned):
- **AuthN/AuthZ** between portal and device:
- **Signing/encryption requirements**:
- **Key management** (factory vs field provisioning):

## 10) Calibration & Drift

### Calibration plan

- **Factory calibration**:
- **On-site calibration**:
- **How often**:
- **Who performs it**:

### Evidence & auditability

- **How calibration is recorded**:
- **What the operator can verify**:

## 11) Compliance / Regulatory (Decisions Needed)

### Legal metrology / “weights & measures”

- **Do we claim exact delivered volume?** yes/no
- **How we frame billing** (prepaid credit decrement vs “measured sale”): 
- **If yes**: required approvals/certifications and timeline:
- **If no**: how we phrase the value proposition and billing basis:

### Compliance action plan (who, what, by when)

- **Target jurisdictions first**:
- **Who we will speak to** (named advisor/regulator contact):
- **Claims we will/won’t make in marketing** (draft wording):
- **Decision needed**: certified flow meter vs “credit decrement” framing vs fixed-price model

### Food safety / materials / cleaning compliance

- Requirements:

## 12) Open Questions & Next Experiments

Add the next smallest experiment that reduces risk.

- **Q**:
  - **Why it matters**:
  - **Proposed experiment**:
  - **Owner**:
  - **Due**:

