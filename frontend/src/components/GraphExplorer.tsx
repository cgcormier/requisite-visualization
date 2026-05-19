import { useEffect, useMemo, useRef } from 'react';
import cytoscape, { type Core, type ElementDefinition } from 'cytoscape';
import type { GraphCommand, GraphResponse } from '../types';

const anyGroupColors = ['#00f5ff', '#39ff14', '#bf5fff', '#ff2bd6', '#ffee00', '#ff8a00', '#4f8cff', '#00ffa3'];

interface GraphExplorerProps {
  command: GraphCommand;
  error: string | null;
  graph: GraphResponse;
  loading: boolean;
  onRetry: () => void;
  onSelectCourse: (courseId: string) => void;
}

function GraphExplorer({ command, error, graph, loading, onRetry, onSelectCourse }: GraphExplorerProps) {
  const containerRef = useRef<HTMLDivElement | null>(null);
  const cyRef = useRef<Core | null>(null);

  const elements = useMemo<ElementDefinition[]>(() => {
    const nodeElements = graph.nodes.map((node) => ({
      data: {
        id: node.id,
        label: node.label || node.id,
        name: node.name ?? 'External prerequisite',
        external: node.external,
        root: node.id === graph.rootCourseId,
      },
    }));

    const edgeElements = graph.edges.map((edge, index) => ({
      data: {
        id: `${edge.from}-${edge.to}-${edge.groupIndex}-${index}`,
        source: edge.from,
        target: edge.to,
        relationship: edge.relationship,
        groupType: edge.groupType,
        groupIndex: edge.groupIndex,
        anyColor: groupColor(edge.groupIndex),
        external: edge.external ?? false,
      },
    }));

    return [...nodeElements, ...edgeElements];
  }, [graph]);

  useEffect(() => {
    if (!containerRef.current) {
      return;
    }

    const cy = cytoscape({
      autoungrabify: false,
      boxSelectionEnabled: false,
      container: containerRef.current,
      elements,
      maxZoom: 2.8,
      minZoom: 0.28,
      selectionType: 'single',
      style: [
        {
          selector: 'node',
          style: {
            'background-color': '#182231',
            'border-color': '#6b7c93',
            'border-width': '2px',
            color: '#f8fafc',
            'font-size': '11px',
            'font-weight': 'bold',
            height: '66px',
            label: 'data(label)',
            'min-zoomed-font-size': '8px',
            shape: 'ellipse',
            'text-halign': 'center',
            'text-max-width': '58px',
            'text-outline-color': '#05070b',
            'text-outline-width': '2px',
            'text-valign': 'center',
            'text-wrap': 'wrap',
            width: '66px',
          },
        },
        {
          selector: 'node[root]',
          style: {
            'background-color': '#00e5ff',
            'border-color': '#f8fafc',
            'border-width': '3px',
            color: '#031018',
            'text-outline-color': '#c6fbff',
            'text-outline-width': '1px',
          },
        },
        {
          selector: 'node[external]',
          style: {
            'background-color': '#080d14',
            'border-color': '#8894a5',
            'border-style': 'dashed',
            color: '#b8c4d4',
          },
        },
        {
          selector: 'edge',
          style: {
            'curve-style': 'bezier',
            'line-color': '#556173',
            opacity: 0.48,
            'target-arrow-color': '#556173',
            'target-arrow-shape': 'triangle',
            width: '2px',
          },
        },
        {
          selector: 'edge[groupType = "any"]',
          style: {
            'line-color': 'data(anyColor)',
            opacity: 0.96,
            'target-arrow-color': 'data(anyColor)',
            width: '3px',
          },
        },
        {
          selector: 'edge[groupType = "all"]',
          style: {
            'line-color': '#667085',
            opacity: 0.38,
            'target-arrow-color': '#667085',
            width: '2px',
          },
        },
        {
          selector: ':selected',
          style: {
            'background-color': '#f8fafc',
            'border-color': '#00f5ff',
            color: '#071019',
            'line-color': '#00f5ff',
            'target-arrow-color': '#00f5ff',
          },
        },
      ],
      layout: graphLayout(),
      userPanningEnabled: true,
      userZoomingEnabled: true,
    });

    cy.on('tap', 'node', (event) => {
      const courseId = event.target.id();
      const isExternal = Boolean(event.target.data('external'));

      if (!isExternal) {
        onSelectCourse(courseId);
      }
    });

    cy.on('tap', (event) => {
      if (event.target === cy) {
        cy.elements().unselect();
      }
    });

    let resizeObserver: ResizeObserver | null = null;

    if (typeof ResizeObserver !== 'undefined') {
      resizeObserver = new ResizeObserver(() => {
        cy.resize();
        window.requestAnimationFrame(() => cy.fit(undefined, 42));
      });
      resizeObserver.observe(containerRef.current);
    }

    cyRef.current = cy;
    window.requestAnimationFrame(() => cy.fit(undefined, 42));

    return () => {
      resizeObserver?.disconnect();
      cy.destroy();
      cyRef.current = null;
    };
  }, [elements, onSelectCourse]);

  useEffect(() => {
    const cy = cyRef.current;

    if (!cy || command.nonce === 0) {
      return;
    }

    if (command.action === 'fit') {
      cy.fit(undefined, 42);
      return;
    }

    if (command.action === 'reset') {
      cy.elements().unselect();
      cy.layout(graphLayout()).run();
      window.requestAnimationFrame(() => cy.fit(undefined, 42));
      return;
    }

    const nextZoom = command.action === 'zoom-in' ? cy.zoom() * 1.18 : cy.zoom() / 1.18;

    cy.zoom({
      level: Math.min(cy.maxZoom(), Math.max(cy.minZoom(), nextZoom)),
      renderedPosition: { x: cy.width() / 2, y: cy.height() / 2 },
    });
  }, [command]);

  return (
    <div className="graph-stage">
      <div className="graph-canvas" ref={containerRef} />

      {loading ? <div className="graph-overlay">Loading graph...</div> : null}

      {error ? (
        <div className="graph-overlay is-error">
          <p>{error}</p>
          <button type="button" onClick={onRetry}>
            Retry
          </button>
        </div>
      ) : null}

      {!loading && !error && graph.nodes.length === 0 ? <div className="graph-overlay">No graph data found.</div> : null}

      <div className="graph-legend" aria-label="Graph legend">
        <span>
          <i className="legend-line required" />
          Required
        </span>
        <span>
          <i className="legend-line alternative" />
          Alternative
        </span>
        <span>
          <i className="legend-node external" />
          External
        </span>
      </div>
    </div>
  );
}

function groupColor(groupIndex: number): string {
  return anyGroupColors[Math.abs(groupIndex) % anyGroupColors.length];
}

function graphLayout() {
  return {
    avoidOverlap: true,
    directed: true,
    name: 'breadthfirst' as const,
    padding: 48,
    spacingFactor: 1.35,
  };
}

export default GraphExplorer;
