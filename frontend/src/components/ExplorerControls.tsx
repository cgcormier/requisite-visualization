import { Maximize2, Minus, Plus } from 'lucide-react';
import type { GraphDirection } from '../types';

interface ExplorerControlsProps {
  direction: GraphDirection;
  depth: number;
  graphNodeCount: number;
  graphEdgeCount: number;
  onDirectionChange: (direction: GraphDirection) => void;
  onDepthChange: (depth: number) => void;
  onFit: () => void;
}

const directions: Array<{ value: GraphDirection; label: string }> = [
  { value: 'prerequisites', label: 'Prereqs' },
  { value: 'both', label: 'Both' },
  { value: 'dependents', label: 'Dependents' },
];

function ExplorerControls({
  direction,
  depth,
  graphNodeCount,
  graphEdgeCount,
  onDirectionChange,
  onDepthChange,
  onFit,
}: ExplorerControlsProps) {
  return (
    <div className="graph-controls">
      <div className="control-cluster">
        <span className="control-label">Direction</span>
        <div className="segmented-control" role="group" aria-label="Graph direction">
          {directions.map((option) => (
            <button
              className={option.value === direction ? 'is-active' : ''}
              key={option.value}
              type="button"
              onClick={() => onDirectionChange(option.value)}
            >
              {option.label}
            </button>
          ))}
        </div>
      </div>

      <div className="control-cluster">
        <span className="control-label">Depth</span>
        <div className="stepper" role="group" aria-label="Graph depth">
          <button
            type="button"
            onClick={() => onDepthChange(Math.max(1, depth - 1))}
            disabled={depth <= 1}
            title="Decrease depth"
            aria-label="Decrease depth"
          >
            <Minus aria-hidden="true" size={16} />
          </button>
          <span>{depth}</span>
          <button
            type="button"
            onClick={() => onDepthChange(Math.min(4, depth + 1))}
            disabled={depth >= 4}
            title="Increase depth"
            aria-label="Increase depth"
          >
            <Plus aria-hidden="true" size={16} />
          </button>
        </div>
      </div>

      <button className="icon-command" type="button" onClick={onFit} title="Fit graph" aria-label="Fit graph">
        <Maximize2 aria-hidden="true" size={17} />
      </button>

      <div className="graph-metrics" aria-label="Visible graph size">
        <span>{graphNodeCount} nodes</span>
        <span>{graphEdgeCount} edges</span>
      </div>
    </div>
  );
}

export default ExplorerControls;
