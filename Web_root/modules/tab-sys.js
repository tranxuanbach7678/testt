// modules/tab-sys.js
import { api } from "./api.js";
import { logActionUI } from "./ui.js";

/**
 * @brief Gui lenh Shutdown hoac Restart
 */
export async function sendPower(act) {
  if (confirm("CHẮC CHẮN " + act.toUpperCase() + " MÁY TÍNH?")) {
    await api("/api/power", { action: act });
    logActionUI("Lệnh nguồn: " + act, true);
  }
}
