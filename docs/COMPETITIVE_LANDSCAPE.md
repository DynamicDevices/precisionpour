# Competitive Landscape (Draft) — PrecisionPour

Purpose: give the team a shared view of competing products/services that overlap with PrecisionPour’s proposal, even when they don’t use the term “self‑pour”.

## Document control

- **Last updated**: 2026-01-25
- **Owner**: TBD
- **Scope**: UK + EU + US (events, stadiums, pubs/bars/taprooms)

## Executive summary (1 page)

PrecisionPour competes with more than “self‑pour beer walls”. Buyers are usually optimizing for **queue time**, **staffing levels**, **quality/foam**, **revenue control**, and **yield**. As a result, competitors fall into several categories:

- **Metered self‑pour platforms (direct competitors)**: authorize → pour → meter → charge (RFID/QR/card).
- **Automated bar kiosks (direct for events)**: turnkey machines for events/venues; often multi‑beverage.
- **Stadium/live‑venue “beer wall” infrastructure**: permanent or deployable automated bars for very high throughput.
- **Fast‑pour systems (substitute)**: reduce queues without changing the payment/account model.
- **Draft monitoring/loss prevention (budget substitute)**: deliver ROI on yield/theft/inventory without self‑serve UX.
- **POS/tab ecosystem platforms**: sit “above” self‑pour to own payments/guest journey and integrate multiple vendors.

## PrecisionPour baseline (for comparison)

Current working assumptions (from our discovery):

- **Business goal**: self‑serve dispensing that reduces queues and staffing while preserving quality and reliability.
- **Commercial model**: lease per month fee + profit share (possibly install fee).
- **Customer flow (MVP‑A)**: QR → mobile portal → enable dispense → pour → metered cost/volume → stop when credit ends.
- **MVP‑B direction**: retrofit into standard hospitality dispense systems (TBD details).

## Category A — RFID/QR/card metered self‑pour platforms (direct competitors)

These map closest to PrecisionPour’s core loop: **authorize** (RFID/QR/terminal) → **open valve** → **meter flow** → **charge** → **reporting**.

### PourMyBeer

- **Positioning**: self‑pour platform with multiple customer interfaces (RFID, QR, tap‑to‑pay/credit card).
- **Notable**: RFID can support prepay or open tab; claims compatibility with POS systems.
- **Sources**: [PourMyBeer RFID interface](https://pourmybeer.com/customer-interface-rfid-technology/)

### iPourIt

- **Positioning**: RFID tokens + interactive tap screens + management workstation; charges by the ounce.
- **Notable**:
  - Supports many beverage types (beer, wine, mixed drinks, cold brew, kombucha, craft soda).
  - Explicit “works without internet” positioning (local database + cloud backup).
  - Publishes a rough per‑tap estimate and per‑ounce subscription fee (beer vs cocktails vs wine).
- **Sources**: [iPourIt FAQ](https://ipouritinc.com/faq/)

### Drink Command

- **Positioning**: RFID or contactless/mobile terminals at taps; “every drop poured is accounted for.”
- **Notable**:
  - Explicit “works without internet” positioning for local operation; internet required for some features/payment authorizations.
  - Strong compliance narrative (pour limits, staff approval flows).
  - Addresses end‑of‑keg foam/run‑out with “Foam On Beer Detectors” (FOBs) to detect empty kegs and alert staff.
- **Sources**: [Drink Command FAQ](https://www.drinkcommand.com/faq/)

## Category B — Automated self‑serve bar kiosks (events & venues)

Often positioned as “automated bar” or “self‑service bar solutions” rather than “self‑pour”.

### Boxbar (UK)

- **Positioning**: automated self‑service bar terminals (bar top, freestanding, container, built‑in wall) + portal + mobile app.
- **Notable**:
  - Multi‑beverage menu (beer/cider/wine/cocktails/spirits+mixers/soft drinks).
  - Strong claims on transaction speed and ops cost reduction.
  - Operates an admin portal and also has a mobile app mentioned on the homepage (“BoxbarGo”).
- **Sources**: [Boxbar homepage](https://www.boxbar.live/), [Boxbar launch post](https://www.boxbar.live/post/boxbar-launches-the-world-first-automated-self-serve-drinks-solution)

### Drinkee (EU festivals)

- **Positioning**: “connected self‑service bar” focused on festivals; story is queue reduction + revenue uplift.
- **Notable**: claims “pour a drink in only 20 seconds” and “increase drink sales by up to 40%”.
- **Sources**: [IMT Starter profile (Drinkee)](https://www.imt-starter.fr/en/startup/drinkee-2)

## Category C — Stadium/live venue “beer wall” infrastructure (UK/EU)

These systems compete on **throughput + quality + operational support**, and can be pitched as capex assets or managed services.

### EBar (UK) — Beerwall + Mobile EBar

- **Positioning**: permanently installed “Beerwall” and deployable “Mobile EBar” for venues and greenfield events.
- **Notable**:
  - Throughput claims and “order>pay>pour” narrative.
  - Explicit operational model (staff can supervise multiple units) + real‑time dashboards.
- **Sources**: [EBar Beerwall](https://www.ebar.co.uk/beerwall-from-ebar-self-service-beer-dispenser/), [EBar Mobile EBar](https://www.ebar.co.uk/mobile-ebar-automatic-beer-dispenser/)

## Category D — “Fast‑pour” & rapid dispense systems (substitutes)

These reduce queues and staffing pressure **without** necessarily implementing per‑ml charging to a user account.

### GS Draft — QuickTap (US)

- **Positioning**: “fast beer taps” with speed, foam/head control, and high yield claims; mobile cart use case.
- **Notable**: emphasizes extreme throughput and yield (“100% keg yield” claims) and operational convenience.
- **Sources**: [QuickTap fast beer taps](https://www.gsdraft.com/quicktap-fast-beer-taps)

## Category E — Fully automated “push‑button” beer dispensers (substitutes)

Automation for speed/consistency; payment may be handled elsewhere.

### Beerjet (EU)

- **Positioning**: automated beer dispenser system for events/stadiums; emphasizes “precision pouring” and “cleaning included”.
- **Sources**: [Beerjet](https://beerjet.com/en/)

### OneTwoBeer (EU)

- **Positioning**: “automatic beer dispenser” where staff (or operator) inserts glass and pushes a button; emphasizes speed and staffing savings.
- **Sources**: [OneTwoBeer](https://www.onetwobeer.com/)

## Category F — “Smart taps” via a consumer app (adjacent)

### IntelliBars / IntelliCup

- **Positioning**: consumer app links to a cup code; machine pours; app processes payment; vendor portal to manage prices/sales.
- **Sources**: [IntelliBars — How it works](https://www.intellibars.com/how-it-works)

## Category G — Draft monitoring, loss prevention, and keg analytics (budget substitutes)

These systems can win budget by proving ROI on **yield**, **theft/waste reduction**, and **inventory visibility**, even if the venue keeps staffed service.

### Kegtron Pro

- **Positioning**: wireless draft inventory management; Swiss flow meters; cloud dashboards; alerts.
- **Notable**: emphasizes “track every keg, every pour” and integrations.
- **Sources**: [Kegtron Pro](https://kegtron.com/pro/)

### US BeerSAVER

- **Positioning**: flow meters + monitoring + reporting; focuses on reducing losses from over‑pours/unaccounted sales.
- **Notable**: emphasizes flow measurement, cloud reporting, and temperature tracking to avoid foamy beer.
- **Sources**: [US BeerSAVER — Flow meter system](https://www.usbeersaver.com/flow-meter-system)

## Category H — POS/tab “platform” ecosystems (partner or competitor)

These companies can own the **payment/tab layer** and integrate with multiple self‑pour systems, potentially commoditizing hardware vendors.

### GoTab (ecosystem example)

- **Positioning**: “self‑pour integration partners” and tab/payment workflows that integrate with multiple self‑pour vendors.
- **Sources**: [GoTab self‑service partners](https://gotab.com/partners/self-service)

## Competitive implications for PrecisionPour (working notes)

- **RFID as a speed path**: many competitors center on RFID for fast venue ops; QR-first is viable, but likely needs an RFID plan for events.
- **Offline strategy**: incumbents explicitly market “still works without internet” for core operations (not always for card auth). Our current discovery defaults to “block unless staff override” during internet/portal outage; this is a strategic choice to revisit.
- **Foam/run‑out is a first‑class problem**: competitors sell dedicated run‑out/foam mitigation (e.g., FOB sensors) and emphasize line cooling and beer quality.
- **Value props split into two archetypes**:
  - **Per‑unit charging + analytics platform** (self‑pour vendors).
  - **Throughput automation** (automated bar/fast‑pour vendors).
  PrecisionPour should be clear which archetype MVP‑B targets first.

## Open items / next research steps

- **UK/EU “tap wall” vendors** beyond the above (bars/taprooms not stadium scale).
- **Pricing**: comparable per‑tap capex, monthly fees, revenue share models, card/RFID cost models.
- **Regulatory positioning**: “weights & measures” vs “prepaid credit decrement” framing and how competitors message compliance.

