import java.util.*;
import java.io.*;

/**
 * Date 11/16/2015
 * @author Tushar Roy
 *
 * Find all cycles in directed graph using Johnson's algorithm
 *
 * Time complexity - O(E + V).(c+1) where c is number of cycles found
 * Space complexity - O(E + V + s) where s is sum of length of all cycles.
 *
 * Link to youtube video - https://youtu.be/johyrWospv0
 *
 * References
 * https://github.com/jgrapht/jgrapht/blob/master/jgrapht-core/src/main/java/org/jgrapht/alg/cycle/JohnsonSimpleCycles.java
 */

class TarjanStronglyConnectedComponent {

    private Map<Vertex<Integer>, Integer> visitedTime;
    private Map<Vertex<Integer>, Integer> lowTime;
    private Set<Vertex<Integer>> onStack;
    private Deque<Vertex<Integer>> stack;
    private Set<Vertex<Integer>> visited;
    private List<Set<Vertex<Integer>>> result;
    private int time;

    public List<Set<Vertex<Integer>>> scc(Graph<Integer> graph) {

        //keeps the time when every vertex is visited
        time = 0;
        //keeps map of vertex to time it was visited
        visitedTime = new HashMap<>();

        //keeps map of vertex and time of first vertex visited in current DFS
        lowTime = new HashMap<>();

        //tells if a vertex is in stack or not
        onStack = new HashSet<>();

        //stack of visited vertices
        stack = new LinkedList<>();

        //tells if vertex has ever been visited or not. This is for DFS purpose.
        visited = new HashSet<>();

        //stores the strongly connected components result;
        result = new ArrayList<>();

        //start from any vertex in the graph.
        for (Vertex<Integer> vertex : graph.getAllVertex()) {
            if(visited.contains(vertex)) {
                continue;
            }
            sccUtil(vertex);
        }

        return result;
    }

    private void sccUtil(Vertex<Integer> vertex) {

        visited.add(vertex);
        visitedTime.put(vertex, time);
        lowTime.put(vertex, time);
        time++;
        stack.addFirst(vertex);
        onStack.add(vertex);

        for (Vertex child : vertex.getAdjacentVertexes()) {
            //if child is not visited then visit it and see if it has link back to vertex's ancestor. In that case update
            //low time to ancestor's visit time
            if (!visited.contains(child)) {
                sccUtil(child);
                //sets lowTime[vertex] = min(lowTime[vertex], lowTime[child]);
                lowTime.compute(vertex, (v, low) ->
                    Math.min(low, lowTime.get(child))
                );
            } //if child is on stack then see if it was visited before vertex's low time. If yes then update vertex's low time to that.
            else if (onStack.contains(child)) {
                //sets lowTime[vertex] = min(lowTime[vertex], visitedTime[child]);
                lowTime.compute(vertex, (v, low) -> Math.min(low, visitedTime.get(child))
                );
            }
        }

        //if vertex low time is same as visited time then this is start vertex for strongly connected component.
        //keep popping vertices out of stack still you find current vertex. They are all part of one strongly
        //connected component.
        if (visitedTime.get(vertex) == lowTime.get(vertex)) {
            Set<Vertex<Integer>> stronglyConnectedComponenet = new HashSet<>();
            Vertex v;
            do {
                v = stack.pollFirst();
                onStack.remove(v);
                stronglyConnectedComponenet.add(v);
            } while (!vertex.equals(v));
            result.add(stronglyConnectedComponenet);
        }
    }
}

class Graph<T>{

    private List<Edge<T>> allEdges;
    private Map<Long,Vertex<T>> allVertex;
    boolean isDirected = false;
    
    public Graph(boolean isDirected){
        allEdges = new ArrayList<Edge<T>>();
        allVertex = new HashMap<Long,Vertex<T>>();
        this.isDirected = isDirected;
    }
    
    public void addEdge(long id1, long id2){
        addEdge(id1,id2,0);
    }
    
    //This works only for directed graph because for undirected graph we can end up
    //adding edges two times to allEdges
    public void addVertex(Vertex<T> vertex){
        if(allVertex.containsKey(vertex.getId())){
            return;
        }
        allVertex.put(vertex.getId(), vertex);
        for(Edge<T> edge : vertex.getEdges()){
            allEdges.add(edge);
        }
    }
    
    public Vertex<T> addSingleVertex(long id){
        if(allVertex.containsKey(id)){
            return allVertex.get(id);
        }
        Vertex<T> v = new Vertex<T>(id);
        allVertex.put(id, v);
        return v;
    }
    
    public Vertex<T> getVertex(long id){
        return allVertex.get(id);
    }
    
    public void addEdge(long id1,long id2, int weight){
        Vertex<T> vertex1 = null;
        if(allVertex.containsKey(id1)){
            vertex1 = allVertex.get(id1);
        }else{
            vertex1 = new Vertex<T>(id1);
            allVertex.put(id1, vertex1);
        }
        Vertex<T> vertex2 = null;
        if(allVertex.containsKey(id2)){
            vertex2 = allVertex.get(id2);
        }else{
            vertex2 = new Vertex<T>(id2);
            allVertex.put(id2, vertex2);
        }

        Edge<T> edge = new Edge<T>(vertex1,vertex2,isDirected,weight);
        allEdges.add(edge);
        vertex1.addAdjacentVertex(edge, vertex2);
        if(!isDirected){
            vertex2.addAdjacentVertex(edge, vertex1);
        }

    }
    
    public List<Edge<T>> getAllEdges(){
        return allEdges;
    }
    
    public Collection<Vertex<T>> getAllVertex(){
        return allVertex.values();
    }
    public void setDataForVertex(long id, T data){
        if(allVertex.containsKey(id)){
            Vertex<T> vertex = allVertex.get(id);
            vertex.setData(data);
        }
    }

    @Override
    public String toString(){
        StringBuffer buffer = new StringBuffer();
        for(Edge<T> edge : getAllEdges()){
            buffer.append(edge.getVertex1() + " " + edge.getVertex2() + " " + edge.getWeight());
            buffer.append("\n");
        }
        return buffer.toString();
    }
}


class Vertex<T> {
    long id;
    private T data;
    private List<Edge<T>> edges = new ArrayList<>();
    private List<Vertex<T>> adjacentVertex = new ArrayList<>();
    
    Vertex(long id){
        this.id = id;
    }
    
    public long getId(){
        return id;
    }
    
    public void setData(T data){
        this.data = data;
    }
    
    public T getData(){
        return data;
    }
    
    public void addAdjacentVertex(Edge<T> e, Vertex<T> v){
        edges.add(e);
        adjacentVertex.add(v);
    }
    
    public String toString(){
        return String.valueOf(id);
    }
    
    public List<Vertex<T>> getAdjacentVertexes(){
        return adjacentVertex;
    }
    
    public List<Edge<T>> getEdges(){
        return edges;
    }
    
    public int getDegree(){
        return edges.size();
    }
    
    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (id ^ (id >>> 32));
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Vertex other = (Vertex) obj;
        if (id != other.id)
            return false;
        return true;
    }
}

class Edge<T>{
    private boolean isDirected = false;
    private Vertex<T> vertex1;
    private Vertex<T> vertex2;
    private int weight;
    
    Edge(Vertex<T> vertex1, Vertex<T> vertex2){
        this.vertex1 = vertex1;
        this.vertex2 = vertex2;
    }

    Edge(Vertex<T> vertex1, Vertex<T> vertex2,boolean isDirected,int weight){
        this.vertex1 = vertex1;
        this.vertex2 = vertex2;
        this.weight = weight;
        this.isDirected = isDirected;
    }
    
    Edge(Vertex<T> vertex1, Vertex<T> vertex2,boolean isDirected){
        this.vertex1 = vertex1;
        this.vertex2 = vertex2;
        this.isDirected = isDirected;
    }
    
    Vertex<T> getVertex1(){
        return vertex1;
    }
    
    Vertex<T> getVertex2(){
        return vertex2;
    }
    
    int getWeight(){
        return weight;
    }
    
    public boolean isDirected(){
        return isDirected;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((vertex1 == null) ? 0 : vertex1.hashCode());
        result = prime * result + ((vertex2 == null) ? 0 : vertex2.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        Edge other = (Edge) obj;
        if (vertex1 == null) {
            if (other.vertex1 != null)
                return false;
        } else if (!vertex1.equals(other.vertex1))
            return false;
        if (vertex2 == null) {
            if (other.vertex2 != null)
                return false;
        } else if (!vertex2.equals(other.vertex2))
            return false;
        return true;
    }

    @Override
    public String toString() {
        return "Edge [isDirected=" + isDirected + ", vertex1=" + vertex1
                + ", vertex2=" + vertex2 + ", weight=" + weight + "]";
    }
}

public class AllCyclesInDirectedGraphJohnson {
    Set<Vertex<Integer>> blockedSet;
    Map<Vertex<Integer>, Set<Vertex<Integer>>> blockedMap;
    Deque<Vertex<Integer>> stack;
    List<List<Vertex<Integer>>> allCycles;

    /**
     * Main function to find all cycles
     */
    public List<List<Vertex<Integer>>> simpleCyles(Graph<Integer> graph) {

        blockedSet = new HashSet<>();
        blockedMap = new HashMap<>();
        stack = new LinkedList<>();
        allCycles = new ArrayList<>();
        long startIndex = 1;
        TarjanStronglyConnectedComponent tarjan = new TarjanStronglyConnectedComponent();
        while(startIndex <= graph.getAllVertex().size()) {
            Graph<Integer> subGraph = createSubGraph(startIndex, graph);
            List<Set<Vertex<Integer>>> sccs = tarjan.scc(subGraph);
            //this creates graph consisting of strongly connected components only and then returns the
            //least indexed vertex among all the strongly connected component graph.
            //it also ignore one vertex graph since it wont have any cycle.
            Optional<Vertex<Integer>> maybeLeastVertex = leastIndexSCC(sccs, subGraph);
            if(maybeLeastVertex.isPresent()) {
                Vertex<Integer> leastVertex = maybeLeastVertex.get();
                blockedSet.clear();
                blockedMap.clear();
                findCyclesInSCG(leastVertex, leastVertex);
                startIndex = leastVertex.getId() + 1;
            } else {
                break;
            }
        }
        return allCycles;
    }

   private Optional<Vertex<Integer>> leastIndexSCC(List<Set<Vertex<Integer>>> sccs, Graph<Integer> subGraph) {
        long min = Integer.MAX_VALUE;
        Vertex<Integer> minVertex = null;
        Set<Vertex<Integer>> minScc = null;
        for(Set<Vertex<Integer>> scc : sccs) {
            if(scc.size() == 1) {
                continue;
            }
            for(Vertex<Integer> vertex : scc) {
                if(vertex.getId() < min) {
                    min = vertex.getId();
                    minVertex = vertex;
                    minScc = scc;
                }
            }
        }

        if(minVertex == null) {
            return Optional.empty();
        }
        Graph<Integer> graphScc = new Graph<>(true);
        for(Edge<Integer> edge : subGraph.getAllEdges()) {
            if(minScc.contains(edge.getVertex1()) && minScc.contains(edge.getVertex2())) {
                graphScc.addEdge(edge.getVertex1().getId(), edge.getVertex2().getId());
            }
        }
        return Optional.of(graphScc.getVertex(minVertex.getId()));
    }

    private void unblock(Vertex<Integer> u) {
        blockedSet.remove(u);
        if(blockedMap.get(u) != null) {
            blockedMap.get(u).forEach( v -> {
                if(blockedSet.contains(v)) {
                    unblock(v);
                }
            });
            blockedMap.remove(u);
        }
    }

    private boolean findCyclesInSCG(
            Vertex<Integer> startVertex,
            Vertex<Integer> currentVertex) {
        boolean foundCycle = false;
        stack.push(currentVertex);
        blockedSet.add(currentVertex);

        for (Edge<Integer> e : currentVertex.getEdges()) {
            Vertex<Integer> neighbor = e.getVertex2();
            //if neighbor is same as start vertex means cycle is found.
            //Store contents of stack in final result.
            if (neighbor == startVertex) {
                List<Vertex<Integer>> cycle = new ArrayList<>();
                stack.push(startVertex);
                cycle.addAll(stack);
                Collections.reverse(cycle);
                stack.pop();
                allCycles.add(cycle);
                foundCycle = true;
            } //explore this neighbor only if it is not in blockedSet.
            else if (!blockedSet.contains(neighbor)) {
                boolean gotCycle =
                        findCyclesInSCG(startVertex, neighbor);
                foundCycle = foundCycle || gotCycle;
            }
        }
        //if cycle is found with current vertex then recursively unblock vertex and all vertices which are dependent on this vertex.
        if (foundCycle) {
            //remove from blockedSet  and then remove all the other vertices dependent on this vertex from blockedSet
            unblock(currentVertex);
        } else {
            //if no cycle is found with current vertex then don't unblock it. But find all its neighbors and add this
            //vertex to their blockedMap. If any of those neighbors ever get unblocked then unblock current vertex as well.
            for (Edge<Integer> e : currentVertex.getEdges()) {
                Vertex<Integer> w = e.getVertex2();
                Set<Vertex<Integer>> bSet = getBSet(w);
                bSet.add(currentVertex);
            }
        }
        //remove vertex from the stack.
        stack.pop();
        return foundCycle;
    }

    private Set<Vertex<Integer>> getBSet(Vertex<Integer> v) {
        return blockedMap.computeIfAbsent(v, (key) ->
            new HashSet<>() );
    }

    private Graph createSubGraph(long startVertex, Graph<Integer> graph) {
        Graph<Integer> subGraph = new Graph<>(true);
        for(Edge<Integer> edge : graph.getAllEdges()) {
            if(edge.getVertex1().getId() >= startVertex && edge.getVertex2().getId() >= startVertex) {
                subGraph.addEdge(edge.getVertex1().getId(), edge.getVertex2().getId());
            }
        }
        return subGraph;
    }

    public static void main(String args[]) {
        AllCyclesInDirectedGraphJohnson johnson = new AllCyclesInDirectedGraphJohnson();
        Graph<Integer> graph = new Graph<>(true);

        try {
 
		
			String pathname = "test_data1.txt"; 
			File filename = new File(pathname);
			InputStreamReader reader = new InputStreamReader(
					new FileInputStream(filename));
			BufferedReader br = new BufferedReader(reader);
			String line = "";
            line = br.readLine();
            int count = 0;
			while (line != null) {
                line = br.readLine();
                String[] input = line.split(",");
                graph.addEdge(Integer.parseInt(input[0]), Integer.parseInt(input[1]));
                count++;
                System.out.println(count);
            }
            

 
		} catch (Exception e) {
			e.printStackTrace();
		}


        /*
        graph.addEdge(1, 2);
        graph.addEdge(1, 8);
        graph.addEdge(1, 5);
        graph.addEdge(2, 9);
        graph.addEdge(2, 7);
        graph.addEdge(2, 3);
        graph.addEdge(3, 1);
        graph.addEdge(3, 2);
        graph.addEdge(3, 6);
        graph.addEdge(3, 4);
        graph.addEdge(6, 4);
        graph.addEdge(4, 5);
        graph.addEdge(5, 2);
        graph.addEdge(8, 9);
        graph.addEdge(9, 8);
        */

        List<List<Vertex<Integer>>> allCycles = johnson.simpleCyles(graph);
        allCycles.forEach(cycle -> {
            StringJoiner joiner = new StringJoiner("->");
            cycle.forEach(vertex -> joiner.add(String.valueOf(vertex.getId())));
            System.out.println(joiner);
        });
    }

}