import {taskState} from "./protocol.js";

const states = ["running", "stopped", "starting", "waiting", "stopping", "failed"];
const label = text => text.charAt(0).toUpperCase() + text.slice(1);
function element(tag, className, text) {
  const el = document.createElement(tag);
  el.className = className;
  if (text !== undefined) el.textContent = text;
  return el;
}
function changedAt(task) {
  const value = task.running ? task.started : task.ended;
  const date = value == null ? null : new Date(value);
  return date && Number.isFinite(date.getTime()) ? date : null;
}
function elapsed(date) {
  if (!date) return "time unknown";
  const seconds = Math.max(0, Math.floor((Date.now() - date.getTime()) / 1000));
  const days = Math.floor(seconds / 86400);
  const hours = Math.floor(seconds / 3600) % 24;
  const minutes = Math.floor(seconds / 60) % 60;
  return days ? `${days}d ${hours}h ${minutes}m` : hours ? `${hours}h ${minutes}m` : minutes ? `${minutes}m ${seconds % 60}s` : `${seconds}s`;
}

// Implements the list operations used by index.js and System, while reusing
// their action callbacks, dialogs, permissions and live protocol updates.
export class ExpandableTasks {
  rows = new Map();
  constructor(parent, object, field, {columns}) {
    this.object = object;
    this.field = field;
    this.actions = columns.filter(column => column.type?.click);
    this.el = element("section", "service-list");
    const toolbar = element("div", "service-filters");
    this.search = element("input", "service-search");
    this.search.type = "search";
    this.search.placeholder = "Find a service…";
    this.search.setAttribute("aria-label", "Find a service");
    this.filter = element("select", "service-filter");
    this.filter.setAttribute("aria-label", "Filter by status");
    for (const state of ["all", ...states]) {
      const option = element("option", "", state === "all" ? "All statuses" : label(state));
      option.value = state;
      this.filter.append(option);
    }
    this.count = element("span", "service-count");
    toolbar.append(this.search, this.filter, this.count);
    this.list = element("div", "service-rows");
    this.empty = element("p", "service-empty", "No matching services.");
    this.el.append(toolbar, this.list, this.empty);
    parent.appendChild(this.el);
    this.search.addEventListener("input", () => this.refresh());
    this.filter.addEventListener("change", () => this.refresh());
    this.refresh();
  }
  makeRow(task) {
    const details = element("details", "service-row");
    const summary = element("summary", "service-summary");
    const chevron = element("span", "service-chevron", "›");
    chevron.setAttribute("aria-hidden", "true");
    const name = element("span", "service-name");
    const status = element("span", "service-status");
    summary.append(chevron, name, status);
    const panel = element("div", "service-panel");
    const changed = element("div", "service-changed");
    const buttons = element("div", "service-actions");
    const error = element("div", "service-error");
    error.setAttribute("role", "status");
    const row = {details, name, status, changed, task, buttons: []};
    for (const action of this.actions) {
      const button = element("button", "service-action", action.name === "Show Log" ? "Logs" : action.name === "Edit" ? "Edit configuration" : action.name);
      button.type = "button";
      button.dataset.action = action.name.toLowerCase();
      button.addEventListener("click", async (event) => {
        // A log button in the summary must not also toggle its details.
        event.preventDefault();
        event.stopPropagation();
        error.textContent = "";
        try { await action.type.click({rowData: row.task}); }
        catch (err) { details.open = true; error.textContent = err.message || String(err); }
      });
      if (action.name === "Show Log") summary.append(button);
      else buttons.append(button);
      row.buttons.push({button, action});
    }
    panel.append(changed, buttons, error);
    details.append(summary, panel);
    this.list.append(details);
    return row;
  }
  refresh() {
    const tasks = this.object[this.field] || [];
    const ids = new Set(tasks.map(task => task.id));
    for (const [id, row] of this.rows) if (!ids.has(id)) {
      row.details.remove();
      this.rows.delete(id);
    }
    let shown = 0;
    for (const task of tasks) {
      let row = this.rows.get(task.id);
      if (!row) this.rows.set(task.id, row = this.makeRow(task));
      row.task = task;
      const state = taskState(task);
      const date = changedAt(task);
      row.name.textContent = task.name || "Unnamed service";
      row.status.textContent = `${label(state)} · ${elapsed(date)}`;
      row.status.dataset.state = state;
      row.changed.textContent = `${task.running ? "Started" : "Stopped"}: ${date ? date.toLocaleString() : "unknown time"}`;
      row.status.title = row.changed.textContent;
      row.details.hidden = !(String(task.name || "").toLowerCase().includes(this.search.value.toLowerCase()) && (this.filter.value === "all" || this.filter.value === state));
      if (!row.details.hidden) shown++;
      for (const {button, action} of row.buttons) {
        button.hidden = action.name === "Run Once" ? !["stopped", "failed"].includes(state) : action.name === "Stop" ? ["stopped", "failed"].includes(state) : false;
      }
    }
    this.count.textContent = `${shown} of ${tasks.length} services`;
    this.empty.hidden = shown !== 0;
  }
  reinit() { this.refresh(); }
  fill() { this.refresh(); }
  deleteRow(task) {
    this.rows.get(task?.id)?.details.remove();
    this.rows.delete(task?.id);
  }
  remove() { this.el.remove(); this.rows.clear(); }
}

