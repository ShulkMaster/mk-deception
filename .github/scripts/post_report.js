// Publish the objdiff report comment bodies written by tools/github_report.py
// (comment-1.md, comment-2.md, ...) to a pull request. Existing report
// comments are edited in place, and leftover parts from an earlier, longer
// report are deleted.
const fs = require('fs');
const path = require('path');

module.exports = async ({ github, context, issueNumber, version, dir }) => {
  const files = fs
    .readdirSync(dir)
    .map(name => ({ name, match: /^comment-(\d+)\.md$/.exec(name) }))
    .filter(entry => entry.match)
    .map(entry => ({ part: Number(entry.match[1]), file: path.join(dir, entry.name) }))
    .sort((a, b) => a.part - b.part);
  if (files.length === 0) {
    throw new Error(`No comment bodies found in ${dir}`);
  }

  // Comments from before the report was split carry the marker without a part
  // number; treat them as part 1.
  const markerPattern = new RegExp(`<!-- objdiff-function-report:${version}(?::(\\d+))? -->`);
  const comments = await github.paginate(github.rest.issues.listComments, {
    ...context.repo,
    issue_number: issueNumber,
    per_page: 100,
  });
  const existing = new Map();
  const extra = [];
  for (const comment of comments) {
    if (comment.user?.type !== 'Bot') continue;
    const match = markerPattern.exec(comment.body ?? '');
    if (!match) continue;
    const part = match[1] ? Number(match[1]) : 1;
    if (existing.has(part)) {
      extra.push(comment);
    } else {
      existing.set(part, comment);
    }
  }

  for (const { part, file } of files) {
    const body = `<!-- objdiff-function-report:${version}:${part} -->\n${fs.readFileSync(file, 'utf8')}`;
    const previous = existing.get(part);
    existing.delete(part);
    if (previous) {
      await github.rest.issues.updateComment({ ...context.repo, comment_id: previous.id, body });
    } else {
      await github.rest.issues.createComment({ ...context.repo, issue_number: issueNumber, body });
    }
  }

  for (const stale of [...existing.values(), ...extra]) {
    await github.rest.issues.deleteComment({ ...context.repo, comment_id: stale.id });
  }
};
