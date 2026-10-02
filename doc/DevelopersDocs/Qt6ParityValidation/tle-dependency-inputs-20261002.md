# TLE inputs and legacy MarsGRAM availability — 2026-10-02

The exact June 2020 catalog was recovered from the original [Thinking Systems R2020 distribution](https://www.thinksysinc.com/archives/GMATR2020aDownloads.html). Only the two newly unblocked runtime stages used `QualifyExamples.child_stage(repo, evidence, entry, 'run', 180)`. No build, passing corpus stage, other TLE example, L2 mission or longrun was repeated. Numerical scripts and objectives were unchanged.

## Exact inputs and staged sources

The preserved [Ubuntu R2020 archive](https://www.thinksysinc.com/downloads/GMATR2020a/GMATR2020a-TlePropagator-Ubuntu20.04.tar.gz) is 400,061 bytes, SHA256 `9ff6ac53dc97e26ab0f67c1b1d0af6f6f102ef8cce422bf3e3a030b7a319ee34`. Its exact `TLE/Active-2020-06-23.txt` is 475,272 bytes, SHA256 `6b89cfa0c5bdde2437c74f6dc28ebaee835537f8159631028efe2abe3e2bac2a`; FALCONSAT-7 is NORAD 44347, element epoch 20174.75800252. The archived November catalog is SHA256 `ea099d5e92abd37d1fd22e70421539f4f48ac4dfcdbb3f486e67f6f97e284967`; its only difference from the checkout is CRLF versus LF. Original distribution bytes were retained.

Ignored `build/example-qualification/20261002/dependency-availability` preserves archive, license/readme, catalogs, member/extraction hashes and HTTP retrieval metadata. The R2020 binary was not extracted, installed or executed. Byte-identical script copies under `samples/NeedTlePropagator` resolve literal `../../TLE/...` references in this owned tree. Original/staged script and input hashes matched before and after both runs.

## Only two runtime results

| Original corpus entry | Result | Duration | Disposition |
| --- | --- | ---: | --- |
| `Falconsat7Jupe.script` | Completed, exit 0 | 1.172 s | Exact historical input at unchanged fixed 23 June 2020 epoch. Engine log confirms completion; both contact reports were generated. |
| `FalconSats.script` | Failed, exit 1 | 0.622 s | Catalog lookup resolved. CSPICE reports `SPICE(BADMECCENTRICITY)`: mean eccentricity -5.3807189001060E-03 outside [-0.001,1.0). |

FalconSats resets all spacecraft epochs using `SystemTime(now)` while retaining 2019/2020 elements. Its failure is consistent with stale-element/current-time extrapolation; the diagnostic does not identify which spacecraft failed. It remains an unexpected raw failure, without current-tracking accuracy, scientific validation, proprietary exception or successful-mission claims. No unchanged retry is scheduled. Logs also retain an invalid-Gregorian warning and CSPICE cleanup warning. Long isolated paths triggered the documented SPICE diagnostic-file fallback to console; all console/stderr/GMAT logs remain preserved.

`targeted-runtime-results.json` and `runtime-retries` in the owned tree preserve actual process executable identity, startup/settings/output isolation, original/staged source identities, exact TLE hashes, configured plugin/runtime inputs, named linked-core snapshots, generated report/log/screenshot hashes and interpretation limits. Both used app SHA256 `7b4a0ae720bb074c71ac05e73d351e987beae5c7fcda524e105d7fb1d55fdae3`. Named core snapshots are base `cf147e234b57818fecf475e0516da86d9187e10066d981e6c893ae7fd761e016` and util `1e4e7fe6ba83701266ef780588003c3ae607b6ebc56ecc040f9cfac399777fcc`. These snapshots do not prove build source or all loader overrides. Offscreen execution does not establish native rendering or independent scientific accuracy.

The ledger update replaces only these two original run entries after exact source-hash matches and preserves their previous missing-file failures and all earlier histories.

## Remaining availability dispositions

- **Falconsat7Contacts:** [CelesTrak SATCAT](https://celestrak.org/satcat/records.php?CATNR=44347) reports decay on 2021-07-02; the current GP endpoint returned HTTP 404. Appropriate current-orbit elements are unavailable for this unchanged current-time scenario. Historical elements are not a valid substitute.
- **GSFCSats / Starlink:** matching 12 December 2019 elements remain outstanding. All 22/117 names are mapped to NORAD numbers in the owned requirements/catalog-number files. [CelesTrak historical access](https://celestrak.org/NORAD/archives/request.php) requires a human name/email/CAPTCHA request, with 100 satellites per request; Starlink needs two requests or its recommended Space-Track bulk route. No request was submitted or mapping catalog substituted as December orbit data. Public inputs remain open.
- **Ex_MarsGRAM2005:** no matching legacy GMAT `libMarsGRAM` plus 2005 FORTRAN namelist/`binFiles` package was found in the bounded local official/primary-source search. NASA lists [Mars-GRAM 2010](https://software.nasa.gov/software/MFS-33158-1) and [modern GRAM Suite](https://software.nasa.gov/software/MFS-33888-1) as General Public Release; the 2010 request leads to [NASA login](https://software.nasa.gov/login). These entries do not establish compatible legacy integration availability. Missing optional integration/data remains unqualified, without a proprietary-only conclusion or model substitution.

## Corpus totals after these stages

| Standalone scope | Missions | Build passed | Build failed | Run completed | Unexpected run failures | Expected tutorial Stop | Timed out |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `application/samples` | 138 | 123 | 15 | 119 | 0 | 1 | 3 |
| Help tutorial downloads | 19 | 17 | 2 | 15 | 1 | 1 | 0 |
| API and TLE sample missions | 7 | 7 | 0 | 3 | 4 | 0 | 0 |
| Total | 164 | 147 | 17 | 137 | 5 | 2 | 3 |

Raw run counts are 137 passes, seven failures and three timeouts; 17 were not attempted because builds failed. Two expected tutorial Stops remain raw failures and are not completed missions. Current child-stage count stays 311. Two new attempts replace current run records and retain prior attempts in history. The refreshed summary, 177-row TSV and current failure excerpts reconcile these source-matched runtime stages; earlier missing-file attempts remain in history.
