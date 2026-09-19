// Proves a built copy of the page actually boots.
//
// CI already proves the engine compiles and the app links. Nothing proves that
// the thing being deployed loads its engine, draws a neck and answers a tap -
// and every way a page like this breaks after a build that passed breaks
// exactly there: a stylesheet that never arrived, a script that never ran. So
// this drives the real built page in a real browser and fails the deploy
// rather than the visitor.
//
//   node web/smoke-test.mjs <directory>
//
// The directory is served over HTTP rather than opened as a file, because that
// is how it is deployed: a service worker needs an origin.

import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { extname, join, normalize } from "node:path";
import { chromium } from "playwright";

const root = process.argv[2];

if (!root) {
  console.error("usage: node web/smoke-test.mjs <directory>");
  process.exit(2);
}

const TYPES = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".json": "application/json",
  ".wasm": "application/wasm",
  ".css": "text/css; charset=utf-8"
};

const server = createServer(async (request, response) => {
  const path = decodeURIComponent(new URL(request.url, "http://x").pathname);

  // The trailing slash has to be read off the request, not off the joined path:
  // join() drops it, and a directory read comes back as a 404 that the browser
  // then offers to download. Which is exactly how this was first found.
  const wanted = path.endsWith("/") ? path + "index.html" : path;
  const file = join(root, normalize(wanted).replace(/^(\.\.[/\\])+/, ""));

  try {
    const body = await readFile(file);
    response.writeHead(200, { "content-type": TYPES[extname(file)] ?? "application/octet-stream" });
    response.end(body);
  } catch {
    response.writeHead(404).end("not found");
  }
});

await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));

const origin = `http://127.0.0.1:${server.address().port}`;

// The environment may keep its browser somewhere other than where Playwright
// looks; honoured when set, ignored when not, so CI needs nothing extra.
const browser = await chromium.launch(process.env.CHROMIUM_PATH
  ? { executablePath: process.env.CHROMIUM_PATH }
  : {});

const page = await browser.newPage();

/*  Records every note the page sounds, so a check can say what was actually
    played rather than only what the page says it did. It runs before any of the
    page's own script, which is what lets the file under test stay the shipped
    one, unmodified. */
await page.addInitScript(() => {
  window.__sounded = [];
  const Ctor = window.AudioContext || window.webkitAudioContext;
  const realCreate = Ctor.prototype.createOscillator;

  Ctor.prototype.createOscillator = function () {
    const osc = realCreate.call(this);
    const realStart = osc.start.bind(osc);

    osc.start = function (when) {
      window.__sounded.push({ hz: osc.frequency.value, when });
      return realStart(when);
    };

    return osc;
  };
});

const failures = [];
let checks = 0;

function check(what, passed) {
  checks++;
  console.log(`${passed ? "  ok  " : "  FAIL"}  ${what}`);
  if (!passed) failures.push(what);
}

page.on("pageerror", (error) => failures.push(`page error: ${error.message}`));

/*  What is drawn on the neck, as "string:fret:label". The page engraves real
    flat and sharp glyphs, which is right on screen and unreadable in a test, so
    they come back here as the b and # anyone would type. */
const shapeOnNeck = () => page.evaluate(() =>
  Array.from(document.querySelectorAll("#neck .dot")).map((dot) => {
    const cell = dot.parentElement;
    const label = dot.textContent.replace(/\u266D/g, "b").replace(/\u266F/g, "#");
    return `${cell.dataset.string}:${cell.dataset.fret}:${label}`;
  }).join(" "));

try {
  await page.goto(origin, { waitUntil: "load" });
  await page.waitForFunction(() =>
    document.getElementById("engineStatus")?.dataset.state === "ready", null, { timeout: 30000 });

  check("the engine starts", true);

  // --- the neck --------------------------------------------------------------
  const rows = await page.locator("#neck .neck-row").count();
  const cells = await page.locator('#neck .neck-row[data-string="0"] button').count();
  check(`the neck is drawn (${rows} strings, ${cells} frets each)`, rows === 6 && cells === 13);

  check("the low string is at the bottom, as it is in your lap",
    (await page.locator("#neck .neck-row").last().getAttribute("data-string")) === "0");

  check("the fret numbers run along underneath",
    (await page.locator("#fretNumbers span").count()) === 13);

  // --- chord practice --------------------------------------------------------
  const firstShape = await shapeOnNeck();
  check(`a chord is under the hand on arrival (${firstShape})`,
    firstShape.includes("1:3:R"));       // the open C: root at the third fret of the 5th string

  check(`the voicing is named (${await page.locator("#voicingName").innerText()})`,
    (await page.locator("#voicingName").innerText()).length > 0);

  check("and the strings are listed with a fingering",
    (await page.locator("#voicingStrings tbody tr").count()) === 6);

  const firstCount = await page.locator("#voicingCount").innerText();
  await page.locator("#anotherVoicing").click();
  const secondShape = await shapeOnNeck();
  const secondCount = await page.locator("#voicingCount").innerText();

  check(`another voicing is a different shape (${firstCount} -> ${secondCount})`,
    secondShape !== firstShape && secondCount !== firstCount);

  // The whole point of the button: the second answer is somewhere else on the
  // neck, not the same grip with a string muted.
  const positionOf = (shape) => Math.min(...shape.split(" ")
    .map((entry) => Number(entry.split(":")[1]))
    .filter((fret) => fret > 0));

  check(`and it is somewhere else on the neck (${positionOf(firstShape)} -> ${positionOf(secondShape)})`,
    positionOf(secondShape) !== positionOf(firstShape));

  await page.locator("#previousVoicing").click();
  check("and Back comes back to it", (await shapeOnNeck()) === firstShape);

  // A different chord, from the menu the engine filled.
  await page.locator('#roots button[data-note="A"]').click();
  await page.locator('#qualities button[data-suffix="m7"]').click();
  await page.waitForFunction(() =>
    document.getElementById("chordTitle").textContent.startsWith("A"));

  const chordTitle = await page.locator("#chordTitle").innerText();
  check(`the menus pick a chord (${chordTitle})`, chordTitle.includes("minor seventh"));

  const amShape = await shapeOnNeck();
  check(`and it is the open Am7 (${amShape})`, amShape.includes("1:0:R") && amShape.includes("4:1:b3"));

  await page.locator("#hearChord").click();
  check(`the chord is strummed, not played at once (${await page.evaluate(() => window.__sounded.length)} notes)`,
    await page.evaluate(() => {
      const starts = window.__sounded.map((note) => note.when).sort((a, b) => a - b);
      return starts.length >= 8 && starts[starts.length - 1] - starts[0] > 0.05;
    }));

  // --- changing the shape yourself -------------------------------------------
  // The first tap takes the neck over from the suggestion and starts from it,
  // so this is a learner nudging one finger rather than building from silence.
  await page.locator('#roots button[data-note="C"]').click();
  await page.locator('#qualities button[data-suffix=""]').click();
  await page.waitForFunction(() =>
    document.getElementById("chordTitle").textContent.startsWith("C "));

  await page.locator('#neck [data-string="0"][data-fret="3"]').click();
  await page.waitForFunction(() =>
    document.getElementById("readingVerdict").textContent.trim() === "C");

  const read = await page.locator("#readingDetail").innerText();
  check(`putting a G underneath a C is read back as one (${read.slice(0, 40)}...)`,
    read.includes("over"));

  await page.locator("#checkAgainst").click();
  await page.waitForFunction(() =>
    document.getElementById("reading").dataset.state.length > 0);

  const measured = await page.locator("#readingVerdict").innerText();
  check(`and measured against the chord asked for (${measured})`, measured.includes("inverted"));

  // A note that is not in the chord at all, which is the other half of it.
  await page.locator('#neck [data-string="5"][data-fret="1"]').click();
  await page.locator("#checkAgainst").click();
  await page.waitForFunction(() =>
    document.getElementById("reading").dataset.state === "wrong");

  check(`a note outside the chord is pointed at (${await page.locator("#readingDetail").innerText()})`,
    (await page.locator("#readingDetail").innerText()).includes("not in C"));

  await page.locator("#clearShape").click();
  check("clearing hands the neck back to the suggested shape",
    (await page.locator("#readingVerdict").innerText()).includes("Nothing yet")
    && (await shapeOnNeck()).includes("1:3:R"));

  // --- the other mode --------------------------------------------------------
  const chordsTitle = await page.title();
  await page.locator("#modeFretboard").click();

  check(`the page changes colour with the mode`,
    await page.evaluate(() => getComputedStyle(document.body).getPropertyValue("--accent").trim()
                              !== "#c07f79"));

  check(`the masthead names the mode (${chordsTitle} / ${await page.title()})`,
    (await page.title()) !== chordsTitle);

  check("the chord panels are put away", await page.locator("#chordPanels").isHidden());

  const named = await page.locator("#neck .dot").count();
  check(`every fret is named (${named} of them)`, named === 78);

  await page.locator("#lightNote").selectOption("A");
  await page.waitForFunction(() =>
    document.getElementById("litSummary").textContent.includes("places"));

  const litSummary = await page.locator("#litSummary").innerText();
  check(`lighting up a note finds every one of it (${litSummary.slice(0, 48)}...)`,
    litSummary.includes("places"));

  check("and the rest of the neck goes quiet behind it",
    (await page.locator("#neck .dot.ghosted").count()) > 0);

  // --- the quiz --------------------------------------------------------------
  await page.locator("#startQuiz").click();
  await page.waitForFunction(() => !document.getElementById("quizLive").hidden);

  const question = await page.locator("#questionText").innerText();
  check(`a question is asked (${question})`, question.length > 0);
  check("with four answers to choose from", (await page.locator("#choices button").count()) === 4);
  check("and the neck says nothing, which is the point",
    (await page.locator("#neck .dot").count()) === 1);

  // Answered the way the engine would answer it: the note is worked out here,
  // from the tuning and the fret the page drew a question mark on, rather than
  // letting the page mark its own homework.
  const noteAsked = () => page.evaluate(() => {
    const names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
    const open = [40, 45, 50, 55, 59, 64];
    const cell = document.querySelector("#neck .dot.asked")?.parentElement;

    if (!cell) return null;

    return names[(open[Number(cell.dataset.string)] + Number(cell.dataset.fret)) % 12];
  });

  const plain = (text) => text.replace(/♯/g, "#").replace(/♭/g, "b").split("/")[0];

  const rightAnswer = await noteAsked();
  const buttons = await page.locator("#choices button").allInnerTexts();
  const wanted = buttons.findIndex((text) => plain(text) === rightAnswer);

  await page.locator("#choices button").nth(wanted).click();
  await page.waitForFunction(() => !document.getElementById("quizReading").hidden);

  check(`answering it is marked right (${await page.locator("#quizVerdict").innerText()})`,
    (await page.locator("#quizReading").getAttribute("data-state")) === "right");

  check("the verdict gives a landmark, not just a tick",
    (await page.locator("#quizDetail").innerText()).length > 12);

  check("the tally counts it", (await page.locator("#tallyRight").innerText()) === "1");

  await page.locator("#nextQuestion").click();
  await page.waitForFunction(() => document.getElementById("quizReading").hidden);
  check("and there is another question after it",
    (await page.locator("#questionText").innerText()).length > 0);

  // A wrong one, to prove the other half.
  const nowAsked = await noteAsked();
  const wrong = (await page.locator("#choices button").allInnerTexts())
    .findIndex((text) => plain(text) !== nowAsked);

  await page.locator("#choices button").nth(wrong).click();
  await page.waitForFunction(() => !document.getElementById("quizReading").hidden);

  check("a wrong answer says what the right one was",
    (await page.locator("#quizReading").getAttribute("data-state")) === "wrong"
    && (await page.locator("#quizDetail").innerText()).length > 12);

  await page.locator("#endQuiz").click();
  await page.waitForFunction(() => !document.getElementById("quizSummaryPanel").hidden);

  const headline = await page.locator("#summaryHeadline").innerText();
  check(`the run is summed up (${headline})`, headline.includes("2"));
  check("in words as well as numbers",
    (await page.locator("#summaryObservations li").count()) > 0);
  check("with a row for each string asked about",
    (await page.locator("#summaryPerString tr").count()) > 0);

  // --- a quiz answered on the neck itself ------------------------------------
  await page.locator("#quizKind").selectOption("find-note");
  await page.locator("#startQuiz").click();
  await page.waitForFunction(() => !document.getElementById("quizLive").hidden);

  check("a find-the-note question is answered on the neck, not from a list",
    (await page.locator("#choices").isHidden()) && (await page.locator("#tapPrompt").isVisible()));

  const target = await page.evaluate(() => {
    const names = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"];
    const open = [40, 45, 50, 55, 59, 64];
    const wanted = document.getElementById("questionText").textContent
      .replace("Where is ", "").replace("?", "").replace(/♯/g, "#").split("/")[0];
    const onString = Number(document.getElementById("questionDetail").textContent
      .match(/(\d)\w\w string/)[1]);
    const index = 6 - onString;

    for (let fret = 0; fret <= 12; fret++)
      if (names[(open[index] + fret) % 12] === wanted) return { string: index, fret };

    return null;
  });

  await page.locator(`#neck [data-string="${target.string}"][data-fret="${target.fret}"]`).click();
  await page.waitForFunction(() => !document.getElementById("quizReading").hidden);

  check(`tapping the right fret answers it (${await page.locator("#quizVerdict").innerText()})`,
    (await page.locator("#quizReading").getAttribute("data-state")) === "right");

  check("and every other place that note lives is shown",
    (await page.locator("#neck .dot.answer").count()) >= 1);

  await page.locator("#endQuiz").click();

  // --- what the record remembers ---------------------------------------------
  // The quiz above ran in standard tuning and was finished, so there is a
  // session to find. Reloaded rather than read out of the same page: the point
  // of a record is that it survives the visit that made it.
  await page.reload({ waitUntil: "load" });
  await page.waitForFunction(() =>
    document.getElementById("engineStatus")?.dataset.state === "ready", null, { timeout: 30000 });

  await page.locator("#modeFretboard").click();
  await page.waitForFunction(() =>
    document.getElementById("progressLine").textContent.includes("session"));

  const record = await page.locator("#progressLine").innerText();
  check(`the record survives a reload (${record.slice(0, 54)}...)`,
    record.includes("session") && record.includes("questions"));

  await page.locator("#showKnown").check();
  await page.waitForTimeout(150);

  const painted = await page.evaluate(() =>
    Array.from(document.querySelectorAll("#neck .dot[data-known]")).map((dot) => dot.dataset.known));

  check(`what is known is painted on the neck (${painted.join(", ") || "nothing"})`,
    painted.length >= 2 && painted.every((band) => ["sure", "solid", "mixed", "shaky"].includes(band)));

  // Knowledge is per tuning, because the same fret is a different note in each.
  await page.locator("#tuning").selectOption("drop-d");
  await page.waitForFunction(() =>
    document.querySelectorAll("#neck .dot[data-known]").length === 0);

  // The session count is one learner's and stays; what empties is the map, and
  // the line says so rather than pretending the history is gone.
  const otherTuning = await page.locator("#progressLine").innerText();
  check(`a record of one tuning says nothing about another (${otherTuning.slice(-34).trim()})`,
    (await page.locator("#neck .dot[data-known]").count()) === 0
    && otherTuning.includes("0 of its"));

  await page.locator("#tuning").selectOption("standard");
  await page.waitForFunction(() =>
    document.querySelectorAll("#neck .dot[data-known]").length > 0);

  check("and comes back when the tuning does",
    (await page.locator("#neck .dot[data-known]").count()) >= 2);

  // Forgetting is a real button that really destroys it, and it asks first.
  page.once("dialog", (dialog) => dialog.accept());
  await page.locator("#forgetProgress").click();
  await page.waitForFunction(() =>
    document.getElementById("progressLine").textContent.includes("Nothing yet"));

  check("forgetting the record forgets it",
    (await page.locator("#neck .dot[data-known]").count()) === 0
    && (await page.evaluate(() => localStorage.getItem("guitarProgress"))) === null);

  // --- other instruments -----------------------------------------------------
  await page.locator("#modeChords").click();
  await page.locator("#tuning").selectOption("bass");
  await page.waitForFunction(() => document.querySelectorAll("#neck .neck-row").length === 4);

  check("a bass is four strings, and gets shapes like anything else",
    (await page.locator("#neck .neck-row").count()) === 4
    && (await page.locator("#voicingStrings tbody tr").count()) === 4);

  await page.locator("#tuning").selectOption("seven-string");
  await page.waitForFunction(() => document.querySelectorAll("#neck .neck-row").length === 7);

  const sevenShape = await shapeOnNeck();
  check(`a seven-string is seven (${sevenShape.slice(0, 40)}...)`,
    (await page.locator("#neck .neck-row").count()) === 7 && sevenShape.length > 0);

  await page.locator("#tuning").selectOption("standard");
  await page.waitForFunction(() => document.querySelectorAll("#neck .neck-row").length === 6);

  // --- the other way round ---------------------------------------------------
  const firstCellRight = await page.evaluate(() =>
    document.querySelector('#neck .neck-row[data-string="0"] button')?.dataset.fret);

  await page.locator("#hand").selectOption("left");
  await page.waitForFunction(() =>
    document.querySelector('#neck .neck-row[data-string="0"] button')?.dataset.fret === "12");

  const firstCellLeft = await page.evaluate(() =>
    document.querySelector('#neck .neck-row[data-string="0"] button')?.dataset.fret);

  check(`a left-handed neck runs the other way (nut at ${firstCellRight} -> ${firstCellLeft})`,
    firstCellRight === "0" && firstCellLeft === "12");

  check("and the fret numbers turn round with it",
    (await page.evaluate(() =>
      document.querySelector("#fretNumbers span")?.textContent)) === "12");

  // Mirrored with a transform, the note names would be mirrored too, which is
  // the reason this is done by reordering cells instead.
  check("the note names are not mirrored",
    await page.evaluate(() => {
      const dot = document.querySelector("#neck .dot");
      return !dot || getComputedStyle(dot).transform.indexOf("-1") === -1;
    }));

  const leftHandShape = await shapeOnNeck();
  check(`the chord is still the same chord (${leftHandShape.slice(0, 30)}...)`,
    leftHandShape.includes("1:3:R"));

  await page.reload({ waitUntil: "load" });
  await page.waitForFunction(() =>
    document.getElementById("engineStatus")?.dataset.state === "ready", null, { timeout: 30000 });

  check("and which way round you play is remembered",
    (await page.evaluate(() => document.body.dataset.hand)) === "left");

  await page.locator("#hand").selectOption("right");

  // --- a different tuning ----------------------------------------------------
  await page.locator("#modeChords").click();
  await page.locator("#tuning").selectOption("drop-d");
  await page.waitForFunction(() =>
    document.querySelector('#neck .neck-row[data-string="0"] .string-label')?.textContent.includes("D"));

  check("changing the tuning redraws the neck and the chord",
    (await page.locator('#neck .neck-row[data-string="0"] .string-label').innerText()).includes("D"));

  await page.locator("#tuning").selectOption("standard");

  // --- the narrow window -----------------------------------------------------
  await page.setViewportSize({ width: 430, height: 860 });
  await page.waitForTimeout(150);

  const overflow = await page.evaluate(() =>
    document.documentElement.scrollWidth - document.documentElement.clientWidth);

  check(`nothing runs off the side of a phone (${overflow}px over)`, overflow <= 1);

  check("the panels stack rather than shrink",
    await page.evaluate(() => getComputedStyle(document.getElementById("chordPanels"))
      .gridTemplateColumns.split(" ").length === 1));

  // --- offline ---------------------------------------------------------------
  await page.setViewportSize({ width: 1200, height: 900 });
  const cached = await page.evaluate(async () => {
    const registration = await navigator.serviceWorker.ready;
    await new Promise((resolve) => setTimeout(resolve, 400));
    const cache = await caches.open("guitar-learning-app");
    const keys = await cache.keys();
    return { active: !!registration.active, files: keys.map((request) => new URL(request.url).pathname) };
  });

  // Asserting on the cache rather than on a reload: the browser's own HTTP
  // cache will happily answer a reload and make a broken worker look fine.
  check(`the offline worker stocked the engine and the page (${cached.files.length} files)`,
    cached.active && cached.files.some((path) => path.endsWith("guitar-engine.js"))
    && cached.files.some((path) => path.endsWith("index.html") || path.endsWith("/")));

  console.log(`\n${checks - failures.length}/${checks} checks passed`);
} catch (error) {
  console.error("\nthe page did not survive the test:", error);
  failures.push(String(error));
} finally {
  await browser.close();
  server.close();
}

if (failures.length) {
  console.error(`\n${failures.length} failed:`);
  failures.forEach((what) => console.error(`  - ${what}`));
  process.exit(1);
}
