// AI Vision — a tiny image-analysis demo on top of the Claude API.
//
// Serves a single page (public/index.html) and one endpoint:
//
//   POST /api/analyze   { image: "data:image/png;base64,...", mode, question }
//
// The answer is streamed back as newline-delimited JSON events:
//   {"type":"text","text":"..."}                     — a chunk of the answer
//   {"type":"done","model":"...","stop_reason":"..."} — finished
//   {"type":"error","message":"..."}                  — something went wrong

import http from "node:http";
import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import Anthropic from "@anthropic-ai/sdk";

const here = path.dirname(fileURLToPath(import.meta.url));
const PORT = Number(process.env.PORT) || 3000;
const MODEL = process.env.CLAUDE_MODEL || "claude-opus-5";

// The API accepts images up to 5 MB each; base64 inflates by ~4/3.
const MAX_IMAGE_BYTES = 5 * 1024 * 1024;
const MAX_BODY_BYTES = Math.ceil(MAX_IMAGE_BYTES * 1.4) + 64 * 1024;
const MEDIA_TYPES = new Set(["image/jpeg", "image/png", "image/gif", "image/webp"]);

const MODES = {
  describe:
    "Describe this image in detail: the subject, setting, notable objects, colours, " +
    "mood and anything unusual. Use a short opening sentence, then a bulleted list.",
  objects:
    "List every distinct object you can identify in this image. For each one give its " +
    "approximate position (e.g. top-left, centre) and how confident you are. Use a Markdown table.",
  text:
    "Transcribe all text visible in this image exactly as written, preserving line breaks " +
    "and layout where practical. If there is no text, say so. Do not add commentary.",
  alt:
    "Write concise alt text (one or two sentences, under 250 characters) for this image, " +
    "suitable for a screen reader. Output only the alt text.",
};

const SYSTEM =
  "You are the analysis engine behind an image-understanding demo. Answer only from what is " +
  "visible in the image; when something is unclear or ambiguous, say so rather than guessing. " +
  "Format answers in GitHub-flavoured Markdown.";

// Resolves credentials from ANTHROPIC_API_KEY (or an `ant auth login` profile).
const client = new Anthropic();

function parseDataUrl(dataUrl) {
  const match = /^data:([a-z/+.-]+);base64,([A-Za-z0-9+/=]+)$/i.exec(dataUrl ?? "");
  if (!match) throw new HttpError(400, "Expected the image as a base64 data URL.");
  const [, mediaType, data] = match;
  if (!MEDIA_TYPES.has(mediaType)) {
    throw new HttpError(415, `Unsupported image type ${mediaType}. Use JPEG, PNG, GIF or WebP.`);
  }
  if (Buffer.byteLength(data, "base64") > MAX_IMAGE_BYTES) {
    throw new HttpError(413, "Image is larger than 5 MB.");
  }
  return { mediaType, data };
}

function buildPrompt(mode, question) {
  if (mode === "ask") {
    const q = String(question ?? "").trim();
    if (!q) throw new HttpError(400, "Type a question about the image.");
    if (q.length > 2000) throw new HttpError(400, "Question is too long (2000 characters max).");
    return q;
  }
  const prompt = MODES[mode];
  if (!prompt) throw new HttpError(400, `Unknown mode "${mode}".`);
  return prompt;
}

class HttpError extends Error {
  constructor(status, message) {
    super(message);
    this.status = status;
  }
}

async function readJson(req) {
  const chunks = [];
  let size = 0;
  for await (const chunk of req) {
    size += chunk.length;
    if (size > MAX_BODY_BYTES) throw new HttpError(413, "Request body too large.");
    chunks.push(chunk);
  }
  try {
    return JSON.parse(Buffer.concat(chunks).toString("utf8"));
  } catch {
    throw new HttpError(400, "Request body must be JSON.");
  }
}

function describeApiError(err) {
  if (err instanceof Anthropic.AuthenticationError) {
    return "The server has no valid Anthropic credentials. Set ANTHROPIC_API_KEY and restart.";
  }
  if (err instanceof Anthropic.RateLimitError) return "Rate limited by the API — try again shortly.";
  if (err instanceof Anthropic.BadRequestError) return `The API rejected the request: ${err.message}`;
  if (err instanceof Anthropic.APIConnectionError) return "Could not reach the Claude API.";
  if (err instanceof Anthropic.APIError) return `Claude API error ${err.status}: ${err.message}`;
  return err.message || "Unexpected error.";
}

async function handleAnalyze(req, res) {
  const body = await readJson(req);
  const image = parseDataUrl(body.image);
  const prompt = buildPrompt(body.mode, body.question);

  res.writeHead(200, {
    "Content-Type": "application/x-ndjson; charset=utf-8",
    "Cache-Control": "no-store",
  });
  const send = (event) => res.write(JSON.stringify(event) + "\n");

  const stream = client.beta.messages.stream({
    model: MODEL,
    max_tokens: 16000,
    thinking: { type: "adaptive" },
    // If a safety classifier declines, re-run on Anthropic's recommended fallback model.
    betas: ["server-side-fallback-2026-07-01"],
    fallbacks: "default",
    system: SYSTEM,
    messages: [
      {
        role: "user",
        content: [
          { type: "image", source: { type: "base64", media_type: image.mediaType, data: image.data } },
          { type: "text", text: prompt },
        ],
      },
    ],
  });

  // Stop paying for tokens nobody will read.
  res.on("close", () => {
    if (!res.writableFinished) stream.abort();
  });

  stream.on("text", (text) => send({ type: "text", text }));

  try {
    const message = await stream.finalMessage();
    if (message.stop_reason === "refusal") {
      send({ type: "error", message: "Claude declined to analyse this image." });
    } else {
      send({ type: "done", model: message.model, stop_reason: message.stop_reason });
    }
  } catch (err) {
    if (err instanceof Anthropic.APIUserAbortError) return;
    console.error(err);
    send({ type: "error", message: describeApiError(err) });
  }
  res.end();
}

async function serveIndex(res) {
  const html = await fs.readFile(path.join(here, "public", "index.html"));
  res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
  res.end(html);
}

const server = http.createServer(async (req, res) => {
  try {
    if (req.method === "GET" && (req.url === "/" || req.url === "/index.html")) {
      await serveIndex(res);
    } else if (req.method === "POST" && req.url === "/api/analyze") {
      await handleAnalyze(req, res);
    } else {
      throw new HttpError(404, "Not found.");
    }
  } catch (err) {
    const status = err instanceof HttpError ? err.status : 500;
    if (status === 500) console.error(err);
    if (!res.headersSent) {
      res.writeHead(status, { "Content-Type": "application/json" });
      res.end(JSON.stringify({ error: status === 500 ? "Internal server error." : err.message }));
    } else {
      res.end();
    }
  }
});

server.listen(PORT, () => {
  console.log(`AI Vision running at http://localhost:${PORT} (model: ${MODEL})`);
});
