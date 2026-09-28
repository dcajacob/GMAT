***************
Example Scripts
***************

Enable the TLEPropagator plugin and select its registered propagator type::

   Create Propagator TleProp;
   TleProp.Type = SPICESGP4;

``TLE`` is the former type name and is no longer registered. A spacecraft's
``Id`` must match an entry in its ``EphemerisName`` catalog. Relative input paths
are resolved from the script's directory. Keep the plugin's ``samples``, ``TLE``
and ``test`` directories in their supplied relative locations.

Bundled inputs
==============

The following tests require no downloads and retain their original satellite
identities, epochs and propagation settings:

* ``test/TLE/BasicPropagation.script`` uses the adjacent ``TleTestElements.txt``.
* ``test/scripts/BasicPropagation.script`` uses ``../TLE/TleTestElements.txt``.
* ``test/scripts/MMS1Propagation.script`` uses ``../TLE/MMS1_Nov-09-2019.txt``.
* ``test/scripts/TdrsPropagation.script`` uses ``../TLE/Tdrs3_Nov-09-2019.txt``.
* ``test/scripts/TerraPropagation.script`` uses ``../TLE/Terra_Nov-09-2019.txt``.

``samples/NeedTlePropagator/PropLightsail2.script`` uses the bundled
``TLE/Active_Nov-09-2019.txt`` (the ``LIGHTSAIL 2`` entry) at its original
8 November 2019 epoch.

Catalogs to supply separately
============================

These inputs are not included. Place them in the plugin's ``TLE`` directory;
the sample scripts refer to that directory as ``../../TLE``. Do not rename an
unrelated catalog to satisfy a missing filename: its satellite entries and
element epochs must suit the script.

* ``Falconsat7Jupe.script`` requires ``Active-2020-06-23.txt`` containing
  ``FALCONSAT-7`` for its 23 June 2020 20:40:35.868 epoch.
* ``FalconSats.script`` also requires that 23 June 2020 catalog for
  ``FALCONSAT-7``. Its ``FALCONSAT-3`` and ``FALCONSAT-6`` inputs use the bundled
  November 2019 catalog. The mission then sets all three epochs to ``now``;
  this historical demonstration does not provide current tracking elements.
* ``Falconsat7Contacts.script`` requires ``active.txt`` containing
  ``FALCONSAT-7`` with elements appropriate to the current time used by ``now``.
* ``GSFCSats.script`` requires ``active.txt`` with all 22 satellite names in its
  ``Id`` assignments (MMS 1--4, TDRS 3 and 5--13, TERRA, AQUA, AURA,
  LANDSAT 7--8, ISS (ZARYA), NUSTAR and TIMED), appropriate to ``now``.
* ``Starlink.script`` requires a historical ``active.txt`` containing every
  satellite in its ``Id`` assignments at its 12 December 2019 epoch, including
  the November 2019 launch. The bundled 9 November catalog predates that launch
  and cannot replace this input.

The dynamic-time contact examples and historical Starlink example have different
requirements despite sharing the name ``active.txt``. Supply the appropriate
catalog separately for each run. Obtaining those external catalogs is outside
the offline tests.
