import React, { useState, useEffect, useRef, useCallback } from 'react';
import { 
  Terminal, Activity, Cpu, HardDrive, Power, X, Minus, CircuitBoard, AlertTriangle 
} from 'lucide-react';
import { 
  LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer, CartesianGrid 
} from 'recharts';

/** * AETHER WEBOS PORTAL v0.1.4
 * Hardened Production Build - Zero-Cascading Version
 */

const THEME = { bg: '#050505', fg: '#00f2ff', border: '#1f2937' };

const APP_CONFIG = [
  { id: 'klog', icon: Terminal, label: 'KLOG' },
  { id: 'monitor', icon: Activity, label: 'MONITOR' },
  { id: 'explorer', icon: CircuitBoard, label: 'EXPLORER' }
];

// --- Hook: Optimized Hardware Sync ---
const useAetherTelemetry = () => {
  const [metrics, setMetrics] = useState({
    system: { uptime: 0, status: 'offline' },
    memory: { used_kb: 0, total_kb: 16384 },
    network: { rx: 0, link: 0 },
    explorer: { devices: 0 }
  });
  const [logs, setLogs] = useState([]);
  const [history, setHistory] = useState([]);
  const ws = useRef(null);

  useEffect(() => {
    ws.current = new WebSocket('ws://localhost:8080');
    ws.current.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        if (data.type === 'DATA') {
          setMetrics(data.payload);
          setHistory(prev => [...prev.slice(-19), { 
            time: new Date().toLocaleTimeString().split(' ')[0], 
            mem: data.payload.memory.used_kb 
          }]);
        } else if (data.type === 'LOG') {
          setLogs(prev => [...prev.slice(-50), data.payload]);
        }
      } catch (e) { console.error("Data Error", e); }
    };
    return () => { if(ws.current) ws.current.close(); };
  }, []);

  return { metrics, logs, history, sendShutdown: () => ws.current?.send("SHUTDOWN_F7") };
};

// --- Hardened Window Frame (No Cascading Renders) ---
const WindowFrame = ({ id, title, children, onClose, onMinimize, onFocus, position, size, zIndex, isActive, isMinimized }) => {
  // Guard against undefined props immediately
  const safePos = position || { x: 50, y: 50 };
  const safeSize = size || { w: 400, h: 300 };

  const [isDragging, setIsDragging] = useState(false);
  const [relPos, setRelPos] = useState({ x: 0, y: 0 });
  
  // Initialize state directly from props to avoid useEffect setState
  const [curPos, setCurPos] = useState(safePos);

  const handleMouseDown = (e) => {
    e.stopPropagation();
    onFocus(id);
    setIsDragging(true);
    setRelPos({ x: e.clientX - curPos.x, y: e.clientY - curPos.y });
  };

  const handleMouseMove = useCallback((e) => {
    if (isDragging) {
      setCurPos({ x: e.clientX - relPos.x, y: e.clientY - relPos.y });
    }
  }, [isDragging, relPos]);

  useEffect(() => {
    if (isDragging) {
      window.addEventListener('mousemove', handleMouseMove);
      window.addEventListener('mouseup', () => setIsDragging(false));
    }
    return () => window.removeEventListener('mousemove', handleMouseMove);
  }, [isDragging, handleMouseMove]);

  if (isMinimized) return null;

  return (
    <div
      style={{
        transform: `translate(${curPos.x}px, ${curPos.y}px)`,
        width: safeSize.w, height: safeSize.h, zIndex: zIndex,
        borderColor: isActive ? THEME.fg : THEME.border
      }}
      className="absolute top-0 left-0 flex flex-col bg-[#0a0a0a]/95 backdrop-blur-md border shadow-2xl overflow-hidden rounded-sm"
      onMouseDown={() => onFocus(id)}
    >
      <div className="h-7 flex items-center justify-between px-2 cursor-move border-b border-gray-800 bg-[#0f0f0f]" onMouseDown={handleMouseDown}>
        <span className="text-[10px] font-mono uppercase text-gray-400">{title}</span>
        <div className="flex gap-1">
          <button onClick={() => onMinimize(id)} className="hover:text-white"><Minus size={12} /></button>
          <button onClick={() => onClose(id)} className="hover:text-red-500"><X size={12} /></button>
        </div>
      </div>
      <div className="flex-1 overflow-auto custom-scrollbar relative">
        {children}
      </div>
    </div>
  );
};

// --- Main Application ---
export default function AetherWebOS() {
  const { metrics, logs, history, sendShutdown } = useAetherTelemetry();
  
  const [windows, setWindows] = useState([
    { id: 'klog', title: 'KERNEL LOG STREAM', isOpen: true, isMinimized: false, zIndex: 1, pos: { x: 50, y: 50 }, size: { w: 600, h: 400 } },
    { id: 'monitor', title: 'AETHER MONITOR', isOpen: true, isMinimized: false, zIndex: 2, pos: { x: 700, y: 50 }, size: { w: 450, h: 350 } }
  ]);
  
  const [activeWindow, setActiveWindow] = useState('klog');

  const focusWindow = (id) => {
    setActiveWindow(id);
    setWindows(prev => prev.map(w => w.id === id ? { ...w, zIndex: 100, isMinimized: false } : { ...w, zIndex: 1 }));
  };

  return (
    <div className="w-full h-screen bg-[#050505] text-gray-300 font-mono overflow-hidden flex flex-col">
      <div className="h-10 border-b border-gray-800 bg-black/90 flex items-center justify-between px-4 z-[5000] shrink-0">
        <div className="flex items-center gap-6">
          <div className="flex items-center gap-2 text-[#00f2ff] font-bold text-sm">
            <Activity size={18} />
            <span>AETHER<span className="text-gray-500 font-normal">OS</span></span>
          </div>
          <div className="text-[10px] text-gray-600 uppercase">UPTIME: {metrics.system.uptime}ms</div>
          <div className={`text-[10px] ${metrics.network.link ? 'text-green-500' : 'text-red-500'}`}>
            {metrics.network.link ? 'LINK UP' : 'LINK DOWN'}
          </div>
        </div>
        <button onClick={sendShutdown} className="text-red-500 hover:bg-red-500/10 p-1 rounded"><Power size={18} /></button>
      </div>

      <div className="flex-1 relative bg-black">
        {windows.map(win => win.isOpen && (
          <WindowFrame
            key={win.id}
            {...win}
            isActive={activeWindow === win.id}
            onClose={() => setWindows(prev => prev.map(w => w.id === win.id ? {...w, isOpen: false} : w))}
            onMinimize={() => setWindows(prev => prev.map(w => w.id === win.id ? {...w, isMinimized: true} : w))}
            onFocus={focusWindow}
          >
            {win.id === 'klog' && (
              <div className="p-2 text-[11px] text-[#00f2ff] leading-tight font-mono whitespace-pre-wrap">
                {logs.map((l, i) => <div key={i}>{l}</div>)}
              </div>
            )}
            {win.id === 'monitor' && (
              <div className="h-full w-full min-h-[200px] p-4">
                <ResponsiveContainer width="99%" height="99%">
                  <LineChart data={history}>
                    <CartesianGrid stroke="#222" vertical={false} />
                    <YAxis hide domain={[0, 16384]} />
                    <Line type="monotone" dataKey="mem" stroke="#00f2ff" dot={false} isAnimationActive={false} />
                  </LineChart>
                </ResponsiveContainer>
              </div>
            )}
          </WindowFrame>
        ))}

        <div className="absolute bottom-6 left-1/2 -translate-x-1/2 flex gap-4 px-6 py-3 bg-black/60 backdrop-blur-xl border border-white/10 rounded-2xl z-[5000]">
          {APP_CONFIG.map((app) => (
            <button 
              key={app.id} 
              onClick={() => setWindows(prev => prev.map(w => w.id === app.id ? {...w, isOpen: true, isMinimized: false} : w))}
              className={`p-2 rounded-lg transition-colors ${activeWindow === app.id ? 'text-[#00f2ff] bg-white/5' : 'text-gray-500'}`}
            >
              <app.icon size={24} />
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}