// modules/api.js
import { store } from "./store.js";

/**
 * @brief Ham "van nang" de goi API den server.
 * @param {string} path Duong dan API (vi du: /api/processes)
 * @param {object} data Du lieu can gui (neu la POST)
 * @returns {Promise<object|Blob>} Tra ve JSON hoac Blob (cho file)
 */
export async function api(path, data) {
  try {
    let opt = { headers: { "X-Client-ID": store.CLIENT_ID } }; // Luon gui ID client
    if (data) {
      opt.method = "POST";
      opt.body = JSON.stringify(data);
    }
    let r = await fetch(path, opt);
    // Tra ve blob (file binary) neu la anh hoac video
    if (path.includes("screenshot") || path.endsWith(".mp4"))
      return await r.blob();
    // Tra ve JSON cho cac API khac
    return await r.json();
  } catch (e) {
    return { error: e.message }; // Tra ve loi neu 'fetch' that bai
  }
}
