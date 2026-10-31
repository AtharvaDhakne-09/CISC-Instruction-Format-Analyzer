"use strict";

const API = {
    list: "/api/instructions",
    comparison: "/api/comparison",
    detail: (id) => `/api/instructions/${encodeURIComponent(id)}`,
};

const FIELD_KEYS = new Set([
    "prefix",
    "opcode",
    "modrm",
    "sib",
    "displacement",
    "immediate",
]);

const state = {
    selectedId: null,
    requestId: 0,
};

function byId(id) {
    return document.getElementById(id);
}

function escapeHtml(value) {
    return String(value ?? "").replace(/[&<>"']/g, (character) => {
        switch (character) {
            case "&":
                return "&amp;";
            case "<":
                return "&lt;";
            case ">":
                return "&gt;";
            case '"':
                return "&quot;";
            case "'":
                return "&#39;";
            default:
                return character;
        }
    });
}

function fieldClass(key) {
    return FIELD_KEYS.has(key) ? `field-${key}` : "";
}

function byteRange(field) {
    const start = field.offset;
    const end = start + field.length - 1;
    if (field.length > 1) {
        return `bytes ${start}\u2013${end}`;
    }
    return `byte ${start}`;
}

async function fetchJson(url) {
    const response = await fetch(url, {
        headers: { Accept: "application/json" },
    });

    if (!response.ok) {
        let detail = "";
        try {
            const body = await response.json();
            if (body && body.error) {
                detail = ` (${body.error})`;
            }
        } catch (ignored) {
            // The response did not contain JSON; the status code is enough.
        }
        throw new Error(`Request failed with status ${response.status}${detail}.`);
    }

    return response.json();
}

function renderInstructionList(summary) {
    const modeBadge = byId("mode-badge");
    const count = byId("instruction-count");
    const notice = byId("instruction-notice");
    const list = byId("instruction-list");

    if (summary.mode) {
        modeBadge.textContent = summary.mode;
    }

    count.textContent = `${summary.count} ${summary.count === 1 ? "instruction" : "instructions"}`;

    if (summary.notice) {
        notice.textContent = summary.notice;
    }

    list.replaceChildren();

    summary.instructions.forEach((instruction) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "instruction-item";
        button.dataset.id = instruction.id;
        button.setAttribute("aria-pressed", "false");

        const main = document.createElement("span");
        main.className = "item-main";

        const assembly = document.createElement("span");
        assembly.className = "item-assembly";
        assembly.textContent = instruction.assembly;

        const hex = document.createElement("span");
        hex.className = "item-hex";
        hex.textContent = instruction.hex;

        const chip = document.createElement("span");
        chip.className = "length-chip";
        chip.textContent = `${instruction.length} B`;

        main.append(assembly, hex);
        button.append(main, chip);
        button.addEventListener("click", () => selectInstruction(instruction.id));
        list.append(button);
    });
}

async function selectInstruction(id) {
    state.selectedId = id;

    document.querySelectorAll(".instruction-item").forEach((item) => {
        const isActive = item.dataset.id === id;
        item.classList.toggle("is-active", isActive);
        item.setAttribute("aria-pressed", String(isActive));
    });

    const panel = byId("detail-panel");
    panel.classList.add("is-loading");
    panel.setAttribute("aria-busy", "true");

    const requestId = ++state.requestId;

    try {
        const detail = await fetchJson(API.detail(id));
        if (requestId !== state.requestId) {
            return;
        }
        renderInstructionDetail(detail);
    } catch (error) {
        if (requestId !== state.requestId) {
            return;
        }
        renderError(panel, "Could not load this instruction", error.message);
    } finally {
        if (requestId === state.requestId) {
            panel.classList.remove("is-loading");
            panel.setAttribute("aria-busy", "false");
        }
    }
}

function encodingSegment(field) {
    const cells = field.bytes
        .split(" ")
        .filter(Boolean)
        .map((byte) => `<span class="byte">${escapeHtml(byte)}</span>`)
        .join("");

    const range = byteRange(field);

    return `
        <div class="field-segment ${fieldClass(field.key)}"
             role="listitem"
             title="${escapeHtml(field.name)}: ${escapeHtml(field.bytes)} (${escapeHtml(range)})">
            <div class="segment-bytes">${cells}</div>
            <div class="segment-caption">
                <span class="segment-name">${escapeHtml(field.name)}</span>
                <span class="segment-offset">${escapeHtml(range)}</span>
            </div>
        </div>
    `;
}

function fieldRow(field) {
    const present = field.present === true;
    const status = present ? `Present \u00b7 ${byteRange(field)}` : "Absent";
    const hex = present
        ? `<code class="field-hex">${escapeHtml(field.bytes)}</code>`
        : "";

    return `
        <article class="field-row ${present ? "is-present" : "is-absent"} ${fieldClass(field.key)}">
            <span class="field-marker" aria-hidden="true"></span>
            <div class="field-body">
                <div class="field-head">
                    <h5>${escapeHtml(field.name)}</h5>
                    <span class="field-status">${escapeHtml(status)}</span>
                    ${hex}
                </div>
                <p class="field-purpose">${escapeHtml(field.purpose)}</p>
                <p class="field-detail">${escapeHtml(field.detail)}</p>
            </div>
        </article>
    `;
}

function renderInstructionDetail(detail) {
    const panel = byId("detail-panel");
    const presentFields = (detail.fields || []).filter((field) => field.present);
    const track = presentFields.map(encodingSegment).join("");
    const rows = (detail.fields || []).map(fieldRow).join("");
    const byteCount = detail.length === 1 ? "byte" : "bytes";

    panel.innerHTML = `
        <div class="detail-top">
            <div class="detail-copy">
                <p class="detail-kicker">Instruction encoding</p>
                <h3 class="detail-title">${escapeHtml(detail.assembly)}</h3>
                <p class="detail-note">${escapeHtml(detail.note)}</p>
            </div>
            <div class="length-pill" aria-label="Instruction length: ${detail.length} ${byteCount}">
                <span class="length-number">${escapeHtml(detail.length)}</span>
                <span class="length-label">${byteCount}</span>
            </div>
        </div>

        <div class="detail-meta">
            <span class="meta-item">
                <span class="meta-label">Machine code</span>
                <code>${escapeHtml(detail.hex)}</code>
            </span>
            <span class="meta-item">
                <span class="meta-label">Processor mode</span>
                <span>${escapeHtml(detail.mode)}</span>
            </span>
        </div>

        <section class="encoding-block" aria-labelledby="encoding-title">
            <div class="block-heading">
                <h4 id="encoding-title">Encoding map</h4>
                <span>One block = one byte; colour = field role</span>
            </div>
            <div class="encoding-track" role="list" aria-label="Byte-by-byte encoding map">
                ${track}
            </div>
        </section>

        <section class="field-block" aria-labelledby="fields-title">
            <div class="block-heading">
                <h4 id="fields-title">Field breakdown</h4>
                <span>Absent fields are shown explicitly</span>
            </div>
            <div class="field-list">${rows}</div>
        </section>
    `;
}

function chartRow(sample, maxLength, fixed) {
    const cells = Array.from(
        { length: Math.max(sample.length, 1) },
        () => '<span class="chart-cell"></span>'
    ).join("");

    const width = Math.max((sample.length / maxLength) * 100, 4);
    const unit = sample.length === 1 ? "byte" : "bytes";
    const extra = sample.word ? ` \u00b7 ${sample.word}` : "";

    return `
        <div class="chart-row">
            <div class="chart-label">
                <span class="chart-name">${escapeHtml(sample.label)}</span>
                <code class="chart-hex">${escapeHtml(sample.hex)}${escapeHtml(extra)}</code>
            </div>
            <div class="chart-bar-wrap"
                 role="img"
                 aria-label="${escapeHtml(sample.label)}: ${sample.length} ${unit}">
                <div class="chart-bar ${fixed ? "fixed" : ""}" style="width: ${width.toFixed(2)}%">
                    ${cells}
                </div>
            </div>
            <span class="chart-value">${sample.length} ${unit}</span>
        </div>
    `;
}

function wordSegment(field) {
    const label = `${field.name} bits ${field.bits} = ${field.value}`;
    return `
        <div class="word-seg"
             style="flex: ${field.width} 1 0"
             tabindex="0"
             title="${escapeHtml(label)}"
             aria-label="${escapeHtml(label)}">
            <span class="word-name">${escapeHtml(field.name)}</span>
            <span class="word-bits">${escapeHtml(field.bits)}</span>
        </div>
    `;
}

function renderComparison(data) {
    const root = byId("comparison-root");
    const variableSamples = data.variableSamples || [];
    const fixedSamples = data.fixedSamples || [];
    const allSamples = [...variableSamples, ...fixedSamples];
    const maxLength = Math.max(4, ...allSamples.map((sample) => sample.length));

    const variableBars = variableSamples
        .map((sample) => chartRow(sample, maxLength, false))
        .join("");
    const fixedBars = fixedSamples
        .map((sample) => chartRow(sample, maxLength, true))
        .join("");

    const fields = data.fixedFields || [];
    const wordDiagram = fields.map(wordSegment).join("");
    const wordValues = fields
        .map((field) => `<code>${escapeHtml(field.name)}=${escapeHtml(field.value)}</code>`)
        .join("<span aria-hidden=\"true\"> &middot; </span>");

    const rows = (data.rows || [])
        .map(
            (row) => `
                <tr>
                    <td>${escapeHtml(row.aspect)}</td>
                    <td>${escapeHtml(row.variable)}</td>
                    <td>${escapeHtml(row.fixed)}</td>
                </tr>
            `
        )
        .join("");

    root.innerHTML = `
        <div class="compare-intro">
            <article class="compare-card variable">
                <span class="compare-tag">Variable length \u00b7 CISC \u00b7 x86</span>
                <h3>${escapeHtml(data.variableTitle)}</h3>
                <p>${escapeHtml(data.variableText)}</p>
            </article>
            <article class="compare-card fixed">
                <span class="compare-tag">Fixed length \u00b7 RISC \u00b7 RV32I</span>
                <h3>${escapeHtml(data.fixedTitle)}</h3>
                <p>${escapeHtml(data.fixedText)}</p>
            </article>
        </div>

        <div class="length-layout">
            <section class="card length-panel" aria-labelledby="variable-chart-title">
                <div class="panel-heading">
                    <h3 id="variable-chart-title">x86 instruction lengths</h3>
                    <span class="panel-count">CISC \u00b7 variable</span>
                </div>
                <p class="panel-sub">Each bar is divided into one cell per byte. The bars are drawn to the same scale.</p>
                <div class="chart">${variableBars}</div>
                <p class="chart-note">${escapeHtml(data.x86RangeNote)}</p>
            </section>

            <section class="card length-panel" aria-labelledby="fixed-chart-title">
                <div class="panel-heading">
                    <h3 id="fixed-chart-title">RISC-V instruction lengths</h3>
                    <span class="panel-count">RISC \u00b7 RV32I</span>
                </div>
                <p class="panel-sub">Every RV32I base instruction uses one 32-bit word, so every bar has exactly four cells.</p>
                <div class="chart">${fixedBars}</div>
                <div class="word-block">
                    <h4>${escapeHtml(data.fixedFieldsLabel)}</h4>
                    <div class="word-diagram">${wordDiagram}</div>
                    <p class="word-values">${wordValues}</p>
                </div>
                <p class="chart-note">Each RISC-V sample above is 4 bytes. The fields inside those 4 bytes change, but the total length does not.</p>
            </section>
        </div>

        <div class="callout">
            <strong>Scope note</strong>
            <p>${escapeHtml(data.scopeNote)}</p>
        </div>

        <section class="card table-panel" aria-labelledby="comparison-table-title">
            <div class="panel-heading">
                <h3 id="comparison-table-title">Encoding comparison</h3>
                <span class="panel-count">x86 vs RV32I</span>
            </div>
            <div class="table-scroll">
                <table>
                    <thead>
                        <tr>
                            <th scope="col">Aspect</th>
                            <th scope="col" class="col-variable">x86 CISC (variable length)</th>
                            <th scope="col" class="col-fixed">RISC-V RV32I (fixed length)</th>
                        </tr>
                    </thead>
                    <tbody>${rows}</tbody>
                </table>
            </div>
        </section>
    `;

    root.setAttribute("aria-busy", "false");
}

function renderError(container, title, message) {
    container.innerHTML = `
        <div class="error-state">
            <h4>${escapeHtml(title)}</h4>
            <p>${escapeHtml(message)}</p>
            <p>Start the C++ backend and refresh the page:</p>
            <code>cisc_analyzer.exe</code>
        </div>
    `;
    container.setAttribute("aria-busy", "false");
}

async function init() {
    const [listResult, comparisonResult] = await Promise.allSettled([
        fetchJson(API.list),
        fetchJson(API.comparison),
    ]);

    if (listResult.status === "fulfilled") {
        renderInstructionList(listResult.value);

        if (listResult.value.instructions.length > 0) {
            const preferred =
                listResult.value.instructions.find(
                    (instruction) => instruction.id === state.selectedId
                ) || listResult.value.instructions[0];
            selectInstruction(preferred.id);
        }
    } else {
        byId("instruction-count").textContent = "unavailable";
        byId("instruction-list").replaceChildren();
        renderError(
            byId("instruction-list"),
            "Instruction data unavailable",
            listResult.reason.message
        );
    }

    if (comparisonResult.status === "fulfilled") {
        renderComparison(comparisonResult.value);
    } else {
        renderError(
            byId("comparison-root"),
            "Comparison data unavailable",
            comparisonResult.reason.message
        );
    }
}

document.addEventListener("DOMContentLoaded", init);
