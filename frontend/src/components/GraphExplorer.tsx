import { useEffect, useMemo, useRef } from 'react';
import cytoscape, { type Core, type ElementDefinition } from 'cytoscape';
import type { GraphResponse } from '../types';

interface GraphExplorerProps {
  graph: GraphResponse;
  fitSignal: number;
  onSelectCourse: (courseId: string) => void;
}

function GraphExplorer({ graph, fitSignal, onSelectCourse }: GraphExplorerProps) {
  const containerRef = useRef<HTMLDivElement | null>(null);
  const cyRef = useRef<Core | null>(null);

  const elements = useMemo<ElementDefinition[]>(() => {
    const nodeElements = graph.nodes.map((node) => ({
      data: {
        id: node.id,
        label: node.label,
        name: node.name ?? 'External prerequisite',
        external: node.external,
        root: node.id === graph.rootCourseId,
      },
    }));

    const edgeElements = graph.edges.map((edge, index) => ({
      data: {
        id: `${edge.from}-${edge.to}-${index}`,
        source: edge.from,
        target: edge.to,
        relationship: edge.relationship,
        groupType: edge.groupType,
      },
    }));

    return [...nodeElements, ...edgeElements];
  }, [graph]);

  useEffect(() => {
    if (!containerRef.current) {
      return;
    }

    const cy = cytoscape({
      container: containerRef.current,
      elements,
      minZoom: 0.45,
      maxZoom: 2.2,
      wheelSensitivity: 0.16,
      style: [
        {
          selector: 'node',
          style: {
            'background-color': '#f8fafc',
            'border-color': '#5b6f82',
            'border-width': '2px',
            color: '#17202a',
            'font-size': '12px',
            'font-weight': 'bold',
            height: '52px',
            label: 'data(label)',
            shape: 'round-rectangle',
            'text-halign': 'center',
            'text-max-width': '84px',
            'text-valign': 'center',
            'text-wrap': 'wrap',
            width: '92px',
          },
        },
        {
          selector: 'node[root]',
          style: {
            'background-color': '#dff3ed',
            'border-color': '#177963',
            'border-width': '3px',
          },
        },
        {
          selector: 'node[external]',
          style: {
            'background-color': '#f2f4f7',
            'border-color': '#a6b0bb',
            'border-style': 'dashed',
            color: '#52606d',
          },
        },
        {
          selector: 'edge',
          style: {
            'curve-style': 'bezier',
            'line-color': '#8a9bad',
            opacity: 0.92,
            'target-arrow-color': '#8a9bad',
            'target-arrow-shape': 'triangle',
            width: '2px',
          },
        },
        {
          selector: 'edge[groupType = "any"]',
          style: {
            'line-color': '#b9872f',
            'line-style': 'dashed',
            'target-arrow-color': '#b9872f',
          },
        },
        {
          selector: 'edge[groupType = "all"]',
          style: {
            'line-color': '#58718a',
            'target-arrow-color': '#58718a',
          },
        },
        {
          selector: ':selected',
          style: {
            'background-color': '#cfe7ff',
            'border-color': '#2563a6',
            'line-color': '#2563a6',
            'target-arrow-color': '#2563a6',
          },
        },
      ],
      layout: {
        name: 'breadthfirst',
        directed: true,
        padding: 34,
        spacingFactor: 1.28,
        avoidOverlap: true,
      },
    });

    cy.on('tap', 'node', (event) => {
      const courseId = event.target.id();
      const isExternal = Boolean(event.target.data('external'));

      if (!isExternal) {
        onSelectCourse(courseId);
      }
    });

    cyRef.current = cy;

    return () => {
      cy.destroy();
      cyRef.current = null;
    };
  }, [elements, onSelectCourse]);

  useEffect(() => {
    const cy = cyRef.current;

    if (!cy) {
      return;
    }

    cy.layout({
      name: 'breadthfirst',
      directed: true,
      padding: 34,
      spacingFactor: 1.28,
      avoidOverlap: true,
    }).run();
    cy.fit(undefined, 34);
  }, [elements]);

  useEffect(() => {
    cyRef.current?.fit(undefined, 34);
  }, [fitSignal]);

  return (
    <div className="graph-stage">
      <div className="graph-canvas" ref={containerRef} />
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

export default GraphExplorer;
