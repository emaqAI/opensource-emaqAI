# AI Vision

A small image-analysis demo built on the Claude API. You add an image by dropping it in, choosing a file or pasting it. Then pick what you want to know, and the answer streams back as it is written.

| Mode | What you get |
|---|---|
| **Describe** | A detailed description of the subject, setting, objects, colours and mood |
| **Find objects** | A table of the objects Claude can see, with rough positions and how confident it is |
| **Read text** | A transcription of any text in the image (OCR) |
| **Alt text** | A short description for screen readers |
| **Ask a question** | Your own question about the image |

## Run it

Requires Node.js 18 or newer and an [Anthropic API key](https://console.anthropic.com/).

```bash
cd ai-vision
npm install
export ANTHROPIC_API_KEY=sk-ant-...
npm start
# → AI Vision running at http://localhost:3000
```

Optional environment variables:

| Variable | Default | |
|---|---|---|
| `PORT` | `3000` | Port the server listens on |
| `CLAUDE_MODEL` | `claude-opus-5` | Which Claude model analyses the images |

## How it works

```
browser ──(image as base64 + mode)──▶ server.js ──▶ Claude Messages API
        ◀──── streamed JSON lines ────           ◀── streamed answer
```

- **`public/index.html`** is the whole front end. It has no build step and no libraries. It checks the image type and shrinks large photos so the long edge is at most 1568 px, which is the size Claude would scale them to anyway. Then it sends the image to the server and renders the streamed Markdown answer. It escapes all HTML first, so the answer cannot run code on the page.
- **`server.js`** is a plain `node:http` server. It validates the upload (JPEG, PNG, GIF or WebP, up to 5 MB). It sends the image and a prompt for the chosen mode to Claude, and relays the answer back as newline-delimited JSON as it streams in.
  - It uses adaptive thinking, so Claude decides how much to think about each image.
  - Server-side fallbacks are on (`fallbacks: "default"`). If a safety classifier declines a request, the API retries it on Anthropic's recommended fallback model instead of failing.
  - If the browser disconnects, the server stops the request to Claude, so you don't pay for an answer nobody reads.

Your API key stays on the server and is never sent to the browser.

## API

`POST /api/analyze`

```json
{ "image": "data:image/png;base64,...", "mode": "describe" }
```

`mode` is one of `describe`, `objects`, `text`, `alt` or `ask`. For `ask`, also send `"question": "..."`.

The response is `application/x-ndjson`, one event per line:

```json
{"type":"text","text":"A tabby cat asleep on "}
{"type":"text","text":"a sunlit windowsill."}
{"type":"done","model":"claude-opus-5","stop_reason":"end_turn"}
```

On failure, the last line is `{"type":"error","message":"..."}`. If the request itself is invalid, the server answers with a JSON `{ "error": "..." }` and a 4xx status.

Try it from the command line:

```bash
IMG=$(base64 -w0 photo.jpg)
curl -N localhost:3000/api/analyze \
  -H 'Content-Type: application/json' \
  -d "{\"image\":\"data:image/jpeg;base64,$IMG\",\"mode\":\"describe\"}"
```
