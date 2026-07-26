#!/usr/bin/env node
// Aggregates Claude Code token usage for this project from the local session
// transcripts it keeps at ~/.claude/projects/<project-hash>/*.jsonl.
// Usage: node scripts/token-usage-report.js
//
// Distinguishes "fresh" tokens (input + output + cache_creation — tokens that
// represent real new work) from cache_read tokens (replayed context, billed
// at a fraction of the base rate) since a raw sum is dominated by cache reads
// in any long-running session and overstates actual cost/work.

const fs = require('fs');
const path = require('path');
const os = require('os');

const PROJECT_DIR_NAME = 'C--Projects-ELVTRGAME';
const dir = path.join(os.homedir(), '.claude', 'projects', PROJECT_DIR_NAME);

const files = fs.readdirSync(dir).filter(f => f.endsWith('.jsonl'));

let grand = { input: 0, output: 0, cacheCreate: 0, cacheRead: 0, count: 0 };
const byModel = {};
const byDate = {};

for (const file of files) {
  const content = fs.readFileSync(path.join(dir, file), 'utf-8');
  for (const line of content.split('\n')) {
    if (!line.trim()) continue;
    let obj;
    try { obj = JSON.parse(line); } catch { continue; }
    const usage = obj.message && obj.message.usage;
    if (!usage) continue;

    const model = obj.message.model || 'unknown';
    const input = usage.input_tokens || 0;
    const output = usage.output_tokens || 0;
    const cacheCreate = usage.cache_creation_input_tokens || 0;
    const cacheRead = usage.cache_read_input_tokens || 0;

    for (const bucket of [grand, byModel[model] ||= { input: 0, output: 0, cacheCreate: 0, cacheRead: 0, count: 0 }]) {
      bucket.input += input;
      bucket.output += output;
      bucket.cacheCreate += cacheCreate;
      bucket.cacheRead += cacheRead;
      bucket.count += 1;
    }

    if (obj.timestamp) {
      const day = obj.timestamp.slice(0, 10);
      const b = byDate[day] ||= { input: 0, output: 0, cacheCreate: 0, cacheRead: 0, count: 0 };
      b.input += input; b.output += output; b.cacheCreate += cacheCreate; b.cacheRead += cacheRead; b.count += 1;
    }
  }
}

const fresh = t => t.input + t.output + t.cacheCreate;
const total = t => fresh(t) + t.cacheRead;
const fmt = n => n.toLocaleString();

console.log('=== TOTAL (project-to-date) ===');
console.log(`Fresh tokens (input+output+cache_creation): ${fmt(fresh(grand))}`);
console.log(`  input: ${fmt(grand.input)}  output: ${fmt(grand.output)}  cache_creation: ${fmt(grand.cacheCreate)}`);
console.log(`Cache-read tokens (replayed context, discounted): ${fmt(grand.cacheRead)}`);
console.log(`Raw total (all types summed): ${fmt(total(grand))}`);
console.log(`Assistant turns: ${fmt(grand.count)}`);

console.log('\n=== BY MODEL (fresh tokens) ===');
for (const [model, t] of Object.entries(byModel).sort((a, b) => fresh(b[1]) - fresh(a[1]))) {
  console.log(`${model}: fresh ${fmt(fresh(t))} (${t.count} turns), cache-read ${fmt(t.cacheRead)}`);
}

console.log('\n=== BY DATE (fresh tokens) ===');
for (const [day, t] of Object.entries(byDate).sort()) {
  console.log(`${day}: fresh ${fmt(fresh(t))}, ${t.count} turns`);
}
