# GitHub-Setup-Anleitung für CZ5000VST

Diese Anleitung führt dich Schritt für Schritt durch das Hochladen dieses
Projekts auf GitHub und das automatische Bauen von VST3/Standalone-Binaries
per GitHub Actions — auch wenn du bisher wenig Erfahrung mit Git/GitHub hast.

---

## 1. Voraussetzungen installieren (einmalig)

- **Git** installieren: https://git-scm.com/downloads
- Einen **GitHub-Account** anlegen: https://github.com/signup
- Optional, aber empfohlen: **GitHub Desktop** (grafische Oberfläche statt
  Kommandozeile): https://desktop.github.com/

Prüfe nach der Installation in einem Terminal:

```bash
git --version
```

---

## 2. Neues Repository auf GitHub anlegen

1. Auf github.com einloggen → oben rechts auf "+" → "New repository".
2. Repository-Name: z. B. `CZ5000VST`
3. Sichtbarkeit: Public (empfohlen, damit GitHub Actions kostenlos läuft)
   oder Private.
4. WICHTIG: Setze KEIN Häkchen bei "Add a README file", "Add .gitignore"
   oder "Choose a license" — diese Dateien sind bereits enthalten und
   würden sonst einen Konflikt beim ersten Push verursachen.
5. Auf "Create repository" klicken.

GitHub zeigt dir danach eine Repository-URL, z. B.:
```
https://github.com/DEIN_BENUTZERNAME/CZ5000VST.git
```

---

## 3. Projekt lokal vorbereiten

1. Entpacke die mitgelieferte ZIP-Datei (CZ5000VST.zip), z. B. nach
   `Dokumente/CZ5000VST`.
2. Öffne ein Terminal in genau diesem entpackten Ordner (der Ordner, der
   direkt CMakeLists.txt, README.md usw. enthält).

---

## 4. Projekt per Git hochladen (Kommandozeile)

```bash
git init
git add .
git commit -m "Initial commit: CZ5000VST virtueller Casio CZ-5000"
git branch -M main
git remote add origin https://github.com/DEIN_BENUTZERNAME/CZ5000VST.git
git push -u origin main
```

Ersetze DEIN_BENUTZERNAME durch deinen GitHub-Benutzernamen.

Beim Push wirst du ggf. nach Zugangsdaten gefragt. GitHub akzeptiert kein
Passwort mehr direkt — nutze einen Personal Access Token
(https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens)
oder GitHub Desktop (siehe Abschnitt 5).

---

## 5. Alternative: GitHub Desktop (ohne Kommandozeile)

1. GitHub Desktop installieren, mit GitHub-Account anmelden.
2. File → Add Local Repository → den entpackten CZ5000VST-Ordner wählen.
3. Falls nötig: "create a repository" klicken.
4. Commit-Kommentar eingeben, "Commit to main" klicken.
5. "Publish repository" klicken, Sichtbarkeit wählen, "Publish Repository".

---

## 6. Automatischen Build (GitHub Actions) prüfen

Das Projekt enthält bereits `.github/workflows/build.yml`. Nach dem Push:

1. Reiter "Actions" auf der Repo-Seite öffnen.
2. Workflow "Build CZ5000VST" läuft automatisch (ca. 10-20 Min., JUCE wird
   beim ersten Mal komplett heruntergeladen und kompiliert).
3. Drei parallele Jobs: Windows, macOS, Linux.
4. Nach grünem Haken: im Workflow-Lauf unter "Artifacts" die fertigen
   .vst3- und Standalone-Dateien für alle drei Betriebssysteme herunterladen
   — ganz ohne eigenen Compiler.

Bei Fehlern (rotes Kreuz): Job anklicken, Fehlermeldung lesen. Häufigste
Ursache: JUCE-API hat sich seit Version 7.0.12 geändert → GIT_TAG in
CMakeLists.txt aktualisieren. Poste die Fehlermeldung, dann helfe ich beim
Beheben.

---

## 7. Lokal bauen (optional)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Details siehe README.md.

---

## 8. Spätere Änderungen hochladen

```bash
git add .
git commit -m "Kurze Beschreibung der Änderung"
git push
```

Jeder Push löst automatisch einen neuen GitHub-Actions-Build aus.

---

## 9. Release mit fertigen Binaries erstellen (optional)

1. Repo-Seite → "Releases" → "Create a new release".
2. Tag eingeben, z. B. v0.1.0.
3. Von GitHub Actions erzeugte .vst3-/Standalone-Dateien per Drag&Drop
   anhängen.
4. "Publish release" klicken.

---

## Zusammenfassung der wichtigsten Befehle

```bash
# Einmalig:
git init
git add .
git commit -m "Initial commit"
git branch -M main
git remote add origin https://github.com/DEIN_BENUTZERNAME/CZ5000VST.git
git push -u origin main

# Bei jeder weiteren Änderung:
git add .
git commit -m "Beschreibung"
git push
```

Viel Erfolg mit deinem virtuellen CZ-5000!
