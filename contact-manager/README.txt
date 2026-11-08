CONTACT MANAGER
Sean McKenna | CS 499 | Milestone Two | November 7, 2026

Purpose
This enhancement expands the CS 320 Contact Service into a persistent, single-user
command-line application. The original coursework is preserved for comparison.

Package contents
  original/        All four original CS 320 Java source and JUnit test files.
  enhanced/        Enhanced Java application, tests, build scripts, and libraries.
  docs/            Architecture, acceptance criteria, security scope, and decisions.
  evidence/        Actual test output, application transcript, and original hashes.

Quick start on macOS or Linux
1. Install/use a JDK version 17 or later; java and javac must be on PATH.
2. Open a terminal in this package's enhanced folder.
3. Run: sh test.sh
4. Run: sh run.sh

The bundled Gson and JUnit libraries allow these commands to work without network
access or Maven. The application creates an owner-only contact-data directory in
the enhanced folder. For another location, pass a dedicated directory whose parent
already exists: sh run.sh /absolute/path/to/new-contact-data
Do not use a shared folder or put real contact data in this submission package.

Menu
  1 Add     Enter ID, first name, last name, phone, and address.
  2 Find    Enter an existing contact ID.
  3 List    Display contacts in case-sensitive contact-ID order.
  4 Edit    Enter an existing ID and all four replacement fields.
  5 Delete  Enter an existing ID, then type YES to confirm deletion.
  0 Exit    Quit. Every successful change has already been saved.

Validation
ID, first name, and last name: 1-10 Java UTF-16 code units.
Address: 1-30 Java UTF-16 code units. Unicode letters are supported.
Text cannot be blank, have leading/trailing whitespace, contain control/format
characters, or contain unmatched UTF-16 surrogates. IDs are case-sensitive.
Phone: exactly 10 ASCII digits; a leading zero is retained. No separators.
Maximum: 1,000 contacts and a 1 MiB UTF-8 JSON file. These are local-demo limits.

Try it with fictional data
Add C1 / Alex / Rivera / 2025550101 / 10 Sample Road.
Exit and reopen. Find C1. Edit it, exit, and reopen to verify the new values.
Delete C1 with YES. A blank or invalid field is rejected without a partial edit.

Storage and recovery
contacts.json is an array of objects with exactly these string fields:
contactId, firstName, lastName, phone, address. Back up this file only after closing
the application. contacts.lock is a persistent lock-file name, not proof that an
instance is still running; the operating system releases its lock on process exit.
Do not remove or replace the lock file while the application is running.

If startup fails, close other application instances and check that the dedicated
directory and store are readable/writable by you. On POSIX systems, the directory
must grant no group/other access (normally 700) and files no group/other access or
execute permission (normally 600). Do not loosen permissions to bypass the check.
On non-POSIX systems, access protection depends on the account's existing ACLs.
If contacts.json is malformed, preserve a copy and restore a known-good backup
while the application is closed. The application will not reset it automatically.
A filesystem without atomic replacement cannot save; use a supported local store.
There is no automatic backup, encryption, multiuser sharing, or cloud sync.

Development and Windows
The Maven pom.xml supports IDE import and standard Maven tests when Maven and its
normal dependency downloads are available. The verified build is the offline shell
workflow above; Maven execution and Windows were not separately tested.
For a Windows PowerShell build, from enhanced:
  New-Item -ItemType Directory -Force build/main | Out-Null
  $sources = Get-ChildItem src/main/java -Recurse -Filter *.java
  javac --release 17 -encoding UTF-8 -cp 'lib/*' -d build/main $sources.FullName
  java -cp 'build/main;lib/*' edu.snhu.contactmanager.Main

Verification
All 8 original tests and 55 enhanced tests passed with no failures or skips.
Four actual CLI processes verified restart persistence, editing, deletion, and
corruption refusal. See evidence/VERIFICATION.txt for environment and limits.
JUnit emits a reflective-access compatibility warning on the installed JDK 26;
it does not change the successful test results. Application compilation passed
-Xlint:all. Java 17 API/bytecode compatibility was checked with --release 17;
execution was on JDK 26.0.1, not on a separately installed JDK 17.

Dependencies and attribution
Gson 2.14.0: https://github.com/google/gson (Apache License 2.0).
JUnit Platform Console Standalone 1.12.2 / Jupiter 5.12.2:
https://junit.org/junit5/ (EPL 2.0 and bundled dependency licenses).
Required dependency licenses are included alongside the jars in enhanced/lib.
Original repository: https://github.com/SeaKing810/CS-320-Software-Test-Automation-QA
