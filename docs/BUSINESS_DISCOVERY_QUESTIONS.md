# PrecisionPour - Business & Product Discovery (Fill-In Template)

Use this document to align on product intent, constraints, and decisions. Keep answers short and concrete; link out to external docs if needed.

## Document Control

- **Date**:
- **Attendees**:
- **Owner**:
- **Status**: Draft / Agreed / Superseded
- **Scope**: Events / Pubs / Home prototype / Other:

## 1) Snapshot Summary (keep current)

- **What we’re building**: A self dispense machine for customer self-pour for businesses, plus the infrastructure and website portal to support the business.
- **Who it’s for**: pubs and events (music festivals and similar); any places where there are numbers of people and there is a desire to limit staffing levels.
- **Why now / why us**: the technology is cheap and available (flow measurement, connectivity, payments) to make this viable now.
- **Current MVP candidate**:
  - **MVP-A**: portable, demoable unit based on a PerfectDraft machine (per-ml billing; optional fixed price dispense) to attract potential investors/partners.
  - **MVP-B**: installable device for standard hospitality beer dispense systems (details TBD by Joel).
- **Next pilot target** (who/where/when): MVP-A in ~2–3 months to demo to potential partners/investors to fund MVP-B.

## 2) Market, Customers, and Success Metrics

### Target customers (ranked)

- **Primary (near-term wedge)**:
- **Secondary**:
- **Future**:

### Operating environment constraints

- **Cleaning / hygiene**:
- **Temperature range**:
- **Install constraints** (gas lines, keg/cask compatibility, footprint, power, network):
- **Throughput expectations** (pours per hour per tap):

### Customer success criteria

- **Quality of service**:
- **Speed of service**:
- **Reliability / uptime expectations**:
- **Ease of use** (first-time user, accessibility):

### Proof points / comparable systems

- **Known examples** (e.g., Brazil/Britain deployments): what worked, what didn’t, what we can copy:
- **Why they were accepted** (regulatory framing, payment rails, operational model):

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

## 3) Product Definition (What the user experiences)

### End-user flow (baseline)

- Scan QR → web portal page (device-specific) → user logs in / creates account → buys tokens/credit → device authorized → user pours → credit decremented by measured ml → valve closes on zero credit or end of pour

### Beverage scope roadmap

- **Near-term beverages**: beer / soft drinks / spirits
- If expanding, what changes (viscosity, carbonation, safety, licensing, brand strategy e.g. separate name)?
- **Future capabilities**: cocktail mixing? (yes/no; prerequisites; why it’s valuable)

### Sampling / “taster” replacement idea

- **Does self-serve allow sampling?** yes/no
- If yes: **rules** (max ml per sample, pricing, abuse controls):
- **UX**: how the user understands price and remaining credit while sampling:

## 4) Pricing, Billing, and MVP Release

### Commercial unit of account (“precision” definition)

- **Charging model**: per ml / fixed price (all-you-can-pour within a window) / per pint / other:
- **Supported serve sizes**: half pint / pint / custom:
- **Target accuracy** (e.g., ±X ml or X%):
- **Repeatability** (pour-to-pour variance):
- **Edge cases** (foam, partial pours, stop/start behavior):
- **Dispute policy** (what happens if user claims under-dispense?):

### Market entry wedge

- **Initial deployment type**: festivals/events / pubs / other:
- **Why this wedge works** (distribution, ROI, urgency, ease of install):
- **Target market release MVP**: what is the smallest “real” release we can pilot with paying users?

### MVP definitions + go/no-go metrics

- **MVP A (per-ml billing)**: what does “done” look like end-to-end?
  - **Go/no-go metrics**: queue time impact, dispute rate, conversion rate, operator interventions/hour
- **MVP B (fixed price)**: what does “fixed” mean (time window / capped ml / one serve)?
  - **Go/no-go metrics**: adoption, perceived fairness, waste/abuse incidents, ops simplicity

MVP-A success criteria (demo should prove):
- Existing PerfectDraft system continues to dispense well (added components don’t ruin the pour).
- Flow meter accurately records dispensed volume.
- Solenoid valve correctly enables/disables dispense.
- Touchscreen UX: scan QR → mobile portal → enable dispense → (pay if necessary) → pour.
- Screen updates live with volume dispensed and cost.
- Metrics are captured and reported to the portal.
- Dispense stops when credits are finished.

MVP-A “definition of done”:
- Modified PerfectDraft keg/hose attachment incorporating flow meter + valve.
- Touchscreen mounted externally on PerfectDraft unit with a workable power setup.
- Portable unit suitable for demos to partners/investors.

Top demo failure modes to avoid (MVP-A):
- Not being able to access the website.
- Not enabling/disabling dispense correctly.
- Not capturing/reporting dispense amounts and costs correctly.

### Pricing options to validate

- **Per-ml billing** (current intent): pros/cons, required accuracy, user trust factors:
- **Fixed price model** (simplify onboarding/acceptance): what exactly is fixed? (time window / max ml cap / specific serve size):
- **Operator controls**: pricing by time of day, beverage type, promotions, refunds:

## 5) Technology Readiness and Architecture

### Technology readiness (current state)

- **Firmware**:
  - Wi‑Fi connect/reconnect implemented (ESP-IDF project).
  - BLE Improv provisioning implemented; needs testing/enhancing to integrate with portal onboarding.
  - Flow meter reading/volume calc: TBD.
  - Valve/solenoid control: TBD.
  - UI screens: QR screen + pouring screen implemented for initial demo.
  - Device↔portal comms: MQTT implemented; protocol + reliability work TBD; may need HTTPS for device comms depending on portal integration.
- **Hardware (MVP-A prototype parts)**:
  - Touchscreen/ESP32 module: 2.8" ESP32-32E display with BLE + Wi‑Fi ([LCDWiki](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)).
  - Flow sensor: 5V hall-effect prototype part (3V3 mismatch risk).
  - Solenoid valve: 5V prototype part (3V3 control mismatch risk; may be too low-flow / small bore).
  - Level shifting: 3V3↔5V converter for prototyping.
  - PerfectDraft hose adapter connector: replaceable part that can be modified by cutting into hose.
  - Power: USB‑C battery pack or USB‑C PSU.
- **Cloud/web applications**:
  - Landing page exists; device-specific QR page/account/login/payments/ledger/enable path/dashboard are TBD (Dionne).

### Hardware boundary / platform plan

- **Prototype platform**: PerfectDraft + ESP32 (touch UI + flow sensor + solenoid)
- **What we’re validating now** (demo goals / learning goals):
- **Custom hardware plan** (likely NXP-based):
- **One-tap-per-device vs multi-tap**:
- **Any expected “hub” component**:

### Deployment formats to consider (pick likely paths)

- **Self-serve station** (staff-supervised, multiple taps, high throughput):
- **Standalone unit** (like petrol-station coffee machines; minimal staffing):
- **Vending-machine style enclosure** (fully enclosed dispensing/payment UX):
- Notes / implications (cleaning, maintenance, fraud, accessibility, footprint):

### Connectivity, backend, and integrations

- **Primary purpose**: authorization / telemetry / remote config / command-control / other:
- **Preferred transport**: MQTT (preferred) / HTTPS / other:
- **Who operates infra**: we manage broker + portal + payments
- **Third-party systems** (research list): hospitality POS / event ticketing-wristbands / payments / venue ops tools

Current comms intent:
- Device↔cloud ideally MQTT; mobile web is HTTPS; if portal cannot integrate with MQTT then device may need to use HTTPS (MVP-A), but production preference remains MQTT.

## 6) Operations, Monitoring, and Inventory

### Operations & servicing (who does what, when it breaks)

- **Install model**:
  - MVP-A demos: Joel (setup), with Alex support.
- **Keg/cask changeover**: steps, time, who performs, what can go wrong
- **Gas/line compatibility**: CO₂/regulators, connectors, any adapters needed
- **Industry-standard fittings**: are there generic pipe/fitting standards we can rely on? what must we support?
- **Cleaning/line maintenance**: required cadence, procedure, and who owns it
- **On-site troubleshooting**: what a non-technical operator can fix in <5 minutes
- **Spare parts**: what must be on hand at events (valve, sensor, PSU, tubing, etc.)

MVP-A demo ops:
- Setup time target: ~5 minutes.
- Fragile demo failure points: wiring + touchscreen/display.
- Cleaning approach: replaceable hose adapter; wash out adapter between demos.

### Data, reporting, and run-out monitoring

- **Operator dashboard must-have**:
- **Reconciliation** (money in vs ml out):
- **Audit trail expectations**:
- **Run-out monitoring**:
  - Required for MVP-B: display/track amount or % left (not minutes).
  - Approach: known full keg volume minus dispensed volume.
  - Initial configuration: standard per device type; later configurable at keg replacement if needed.
- **Low keg policy**:
  - When “low”: stop/disallow dispense + flag portal.
  - Staff override needed (idea: NFC card + PIN). Strictness policy TBD (Joel/Alex).
- **Dual vessel / switchover**:
  - MVP-B v1: no (manual keg replacement).

## 7) Payments, Identity, and App Strategy

### Payment rails / access methods

- **QR code usage**: primary = payment entry point; secondary = onboarding/support/device info
- **Other options**: RFID/NFC wristbands/cards; “tap to pay”/wallet; staff-issued sessions

### Identity & access control

- **Account required?** yes (account required; staff override exists).
- **Age verification**: must be demonstrated for pilots; enforced via website account.

### App strategy

- **Do we plan a mobile app?** yes/no
- If yes: **who uses it** (operators vs end-users), and **why** (provisioning, payments, support, monitoring)?

### Provisioning/onboarding for operators

- **Preferred**: Improv BLE via custom mobile app
- **Fallback** (if BLE unavailable):

App plan (initial):
- Mobile-first website to avoid requiring app install (for end users).

## 8) Reliability, Safety, and “Degraded Mode” Rules

### Availability expectations

- **Event scenario uptime target**: no more than 5 minutes downtime/day.
- **Max acceptable “time to recover”** if a tap fails: 5 minutes.
- **Scan→authorized latency target**: 3 seconds after payment success.

### Safety rules (define the hard invariants)

- **Valve fail state on power loss**: closed (no dispense on power loss).
- **Watchdog/reset behavior**:
- **Stuck-open detection/mitigation**:
- **What happens on network drop mid-pour**:

### Offline / degraded-mode policy (decide now, test early)

For each scenario, define **what the device does**, **what the user sees**, and **how money/credit is handled**.

- **Internet down**: block pours unless staff override.
- **Portal/backend down**: block pours unless staff override.
- **MQTT down but internet up**: treat as comms failure for device purposes (block unless staff override).
- **Power brownout / reboot mid-pour**:
  - Preferred behavior: (A) void the session, require staff override before any further pours, and refund last session automatically (TBD Alex/Joel).
  - Persist critical inventory state to NVM (e.g., remaining ml in keg).
- **Flow sensor fault mid-pour**: immediately stop, flag, staff override only.

## 9) Security, Fraud, and Abuse

### Threat model (practical)

- **QR label copying / swapping**:
- **Replay of “enable” messages**:
- **Device impersonation**:
- **Physical bypass** (valve, flow sensor, handle switch):
- **Account sharing / resale of tokens**:

Fraud/abuse top 3 priorities: TBD (Alex/Joel)

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

## 10) Calibration, Compliance, and Legal

### Calibration & drift

- **Calibration approach (pilots)**:
  - Manual calibration process initially (Alex).
  - Factory-only calibration process for pilots; periodic servicing with recalibration as part of the business service process.
- **Calibration evidence to log**: calibration date, operator, known-volume test result.
- **Drift handling**: handled via scheduled servicing process (details TBD).

### Legal metrology / “weights & measures”

- **Do we claim exact delivered volume?** yes/no
- **How we frame billing** (prepaid credit decrement vs “measured sale”): prepaid credits added to a website account via mobile payment (Apple Pay, card, etc.).
- **If yes**: required approvals/certifications and timeline:
- **If no**: how we phrase the value proposition and billing basis:

### Compliance action plan (who, what, by when)

- **Target jurisdictions first**:
- **Who we will speak to** (named advisor/regulator contact):
- **Claims we will/won’t make in marketing** (draft wording):
- **Decision needed**: certified flow meter vs “credit decrement” framing vs fixed-price model

### Food safety / materials / cleaning compliance

- Requirements:

## 11) Business Plan, Funding, Partners, Support

### Unit economics / ROI

- **Who pays**: venue / festival operator / brand / distributor / other:
- **How they justify it** (staff savings, throughput uplift, sales uplift, reduced waste):
- **Target hardware cost per tap**:
- **Target gross margin**:
- **Target payback period** (months):

### Commercial / rollout model

- **Model**: lease per month fee plus profit share (possibly installation fee).
- **Who owns hardware on-site**:
- **Who supplies/owns the beverage** (venue vs distributor vs us):
- **Rollout plan**: pilots → paid pilots → scale (what gates each step):

### Development plan to series release

- **Costed development plan**: do we have a costed plan to reach “series release” level? scope/timeline/budget:
- **Target date for pilot 1 / pilot 2**:

### Funding routes

- Which routes have been explored (bootstrapping, angels, grants, strategic partners, venture, revenue-share)? TBD (Andy)

### Brand partnerships & hospitality partners

- **Strategy**: brewer-sponsored taps / venue-owned generic infrastructure / hybrid:
- **Requirements**: branding on-device, promo pricing, data sharing, co-marketing:
- **Hospitality partners**: any identified partners (venues, operators, suppliers)? who can introduce us? TBD (Joel)

### Customer support & disputes

- **Refund policy**: when, how, who authorizes (self-serve vs staff override)
- **Dispute handling** (“paid but stopped early”): required evidence (logs, ml count, timestamps)
- **Support channels**: on-device help, web help, operator hotline, other:

### Liability & insurance

- **Over-serving / intoxication liability**: who is responsible and how is it controlled?
- **Underage service**: what controls are required (wristbands, ID checks, supervised mode)?
- **Spills / injury / equipment damage**: who is liable?
- **Insurance requirements**: what coverage do operators expect/require?

## 12) IP (Intellectual Property)

- **Identified IP**: TBD (Alex/Andy)
- **Status**: no formal steps yet (no trademark search/filing, no provisional patent, no assignments yet)
- **Ownership**: TBD (company/contractors; confirm assignments needed)

## 13) Open Questions & Next Experiments

Add the next smallest experiment that reduces risk.

- **Q**:
  - **Why it matters**:
  - **Proposed experiment**:
  - **Owner**:
  - **Due**:
