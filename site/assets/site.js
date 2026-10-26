// R080 Timer site — shared top bar, menu and IT/EN language switch.
(function () {
  const PAGES = [
    ["index.html", "Panoramica", "Overview"],
    ["pinout.html", "Pinout", "Pinout"],
    ["funzioni.html", "Funzioni e comandi", "Features and controls"],
    ["flussi.html", "Flussi", "Flows"],
    ["installazione.html", "Installazione", "Setup"]
  ];
  const root = document.documentElement;
  const here = document.body.dataset.page || "index.html";

  function stored() {
    try { return localStorage.getItem("r080-lang"); } catch (e) { return null; }
  }
  function store(v) {
    try { localStorage.setItem("r080-lang", v); } catch (e) { /* storage blocked: keep in memory only */ }
  }

  function setLang(lang) {
    root.dataset.lang = lang;
    root.lang = lang;
    const t = root.dataset["title" + (lang === "it" ? "It" : "En")];
    if (t) document.title = t;
    document.querySelectorAll(".langs button").forEach((b) => b.setAttribute("aria-pressed", String(b.dataset.lang === lang)));
    document.dispatchEvent(new CustomEvent("r080:lang", { detail: lang }));
  }

  // top bar
  const bar = document.createElement("header");
  bar.className = "topbar";
  bar.innerHTML =
    '<div class="topbar-in">' +
    '<a class="brand" href="index.html"><span class="dot" aria-hidden="true"></span>R080 Timer</a>' +
    '<ul class="menu">' +
    PAGES.map(([href, it, en]) =>
      `<li><a href="${href}"${href === here ? ' aria-current="page"' : ""}><span lang="it">${it}</span><span lang="en">${en}</span></a></li>`
    ).join("") +
    "</ul>" +
    '<div class="langs" role="group" aria-label="Lingua / Language">' +
    '<button type="button" data-lang="it">IT</button><button type="button" data-lang="en">EN</button>' +
    "</div></div>";
  document.body.prepend(bar);
  bar.querySelectorAll(".langs button").forEach((b) =>
    b.addEventListener("click", () => { store(b.dataset.lang); setLang(b.dataset.lang); }));

  const initial = stored() || ((navigator.language || "it").toLowerCase().startsWith("it") ? "it" : "en");
  setLang(initial);
  window.R080 = { lang: () => root.dataset.lang, setLang };
})();
