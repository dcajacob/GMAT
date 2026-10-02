# Help walkthrough resource transaction fixes — 2026-10-02

Actual private GUI construction of Help Tutorial 4 exposed three resource-edit
failures and a new-propagator workflow gap. These changes address those observed
steps; Tutorial 4 execution/reference comparison and broader Linux acceptance
remain separate gates. Windows/macOS/MATLAB remain deferred.

## Observed failures and changes

- New MainTank rejected Help's valid FuelMass 1718 kg, FuelDensity 1000 kg/m³,
  Volume 2 m³ and Pressure 5000 settings. Alphabetical setters initialized the
  old tank geometry before applying the new mass/volume. Qt now applies the
  coupled mass, geometry and pressure settings to its detached preview in a
  valid order, then validates/initializes the final state. Rejected settings
  retain the original source/resource and pending edits.
- Local MOI burn Origin offered Earth alone, blocking the specified Mars origin.
  Qt now supplies celestial-body references for Local burn/thruster Origin;
  existing typed reference controls expose Mars and Luna as well as Earth.
- DeepSpace Apply accepted PrinceDormand78 and a selected force model but their
  names were lost when the engine serialized/copied the old owned propagator
  and ODE model. Qt now replaces the owned integrator/model on the detached
  preview before applying dependent settings. A changed Type is serialized
  before existing integrator settings so interpretation preserves them. Unsafe
  source relocation rejects the transaction; readback verifies Type/FM.
- Newly created numerical Qt propagators used the shared implicit InternalODEModel,
  preventing a local model rename and exposing accidental sharing. Their default
  now has a unique `<propagator>_ForceModel` declaration/link with the wx Earth/
  JGM2 gravity defaults. The preview remains unregistered. Explicit configured
  model choices, including an existing InternalODEModel, retain sharing; imported
  implicit propagators remain intact. Analytical propagators get no default FM.

These are Qt ordering/ownership/serialization and editor-metadata changes.
No numerical-engine algorithms were changed.

## Focused verification

| Check | Result | Coverage |
|---|---|---|
| QtGui.ChemicalTankApply | Passed, 0.27 s | Valid grouped Create/Apply, invalid rollback, explicit BlowDown and unchanged pressure, source/Undo/Redo/Unicode save-reopen |
| QtGui.BurnOrigin | Passed, 0.20 s | Body-only choices, Local Mars pending/Cancel/Apply, retained source and Unicode save-reopen |
| QtGui.PropSetupApply | Passed, 0.19 s | Actual pending Type/FM controls, numerical settings retention and ordering, invalid rollback, unrelated resource rebuild, exact Undo/Redo/save-reopen |
| QtGui.PropSetupCreation | Passed, 0.36 s | Actual Create after name changes, unregistered preview/Cancel, independent wx default models/edit isolation, explicit existing model, collision/rejection protection, source/Undo/Redo/save-reopen and SPK creation without automatic FM |

Only these four new offscreen GUI transaction checks ran. Creation's first
0.76 s attempt threw an unhandled GmatBaseException; the diagnostic 0.33 s attempt
identified a test assertion using GetStringParameter for the ON_OFF_TYPE SRP.
The test now uses GetOnOffParameter; only that target/check was rebuilt/rerun.
Both failures remain retained. No mission propagation or scientific SPK result
is claimed by the creation test. Existing successful matrices/corpus were not
repeated. Actual affected Tutorial 4 steps continue on the rebuilt app.

Both application and four selected test-target builds passed. Runtime:

- application/bin/GmatQt-R2026a (GmatQt symlink target): SHA256
  `7fb5bd6cdf8c2630599a2f8c4d20c5f1ce680c41f297ab8fc1ac158b4664f6d4`.
- Numerical base remains `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016`;
  util remains `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`.
- Selected startup remains `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a`.

Build logs, all check attempts, final full creation output, source/runtime hashes,
working production patch and artifact index are preserved in ignored
`build/example-qualification/20261002/tutorial-resource-fixes`.
The diagnosis notes and unchanged failure screenshots remain with Tutorial 4's
own private-session evidence. This checkpoint does not qualify the host GNOME
crash, physical graphics drivers, all native lifecycle cases or full replacement.
